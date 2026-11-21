#include "suborno/platform/windows/app_paths.hpp"

#include <ShlObj.h>
#include <array>
#include <sqlite3.h>
#include <string>
#include <system_error>

namespace suborno::platform::windows {
namespace {

class MigrationLock final {
  public:
    ~MigrationLock() {
        if (handle_ != nullptr) {
            if (owned_) {
                ::ReleaseMutex(handle_);
            }
            ::CloseHandle(handle_);
        }
    }

    HRESULT acquire() {
        handle_ = ::CreateMutexW(nullptr, FALSE, L"Local\\Suborno.UserDataMigration");
        if (handle_ == nullptr) {
            return HRESULT_FROM_WIN32(::GetLastError());
        }
        const DWORD wait_result = ::WaitForSingleObject(handle_, INFINITE);
        if (wait_result != WAIT_OBJECT_0 && wait_result != WAIT_ABANDONED) {
            return wait_result == WAIT_FAILED ? HRESULT_FROM_WIN32(::GetLastError()) : E_FAIL;
        }
        owned_ = true;
        return S_OK;
    }

  private:
    HANDLE handle_ = nullptr;
    bool owned_ = false;
};

HRESULT get_folder_path(REFKNOWNFOLDERID folder_id,
                        std::filesystem::path& directory,
                        DWORD flags = KF_FLAG_CREATE) noexcept {
    PWSTR folder = nullptr;
    const HRESULT result = ::SHGetKnownFolderPath(folder_id, flags, nullptr, &folder);
    if (FAILED(result)) {
        return result;
    }
    try {
        directory = std::filesystem::path(folder) / L"Suborno";
    } catch (const std::bad_alloc&) {
        ::CoTaskMemFree(folder);
        return E_OUTOFMEMORY;
    }
    ::CoTaskMemFree(folder);
    return S_OK;
}

HRESULT ensure_directory_exists(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    return error ? HRESULT_FROM_WIN32(static_cast<DWORD>(error.value())) : S_OK;
}

HRESULT make_temporary_path(const std::filesystem::path& destination,
                            std::filesystem::path& temporary) {
    GUID id{};
    const HRESULT result = ::CoCreateGuid(&id);
    if (FAILED(result)) {
        return result;
    }
    std::array<wchar_t, 40> id_text{};
    if (::StringFromGUID2(id, id_text.data(), static_cast<int>(id_text.size())) == 0) {
        return E_FAIL;
    }
    temporary = destination;
    temporary += L".migration";
    temporary += id_text.data();
    return S_OK;
}

HRESULT migrate_settings(const std::filesystem::path& source,
                         const std::filesystem::path& destination) {
    const DWORD destination_attributes = ::GetFileAttributesW(destination.c_str());
    if (destination_attributes != INVALID_FILE_ATTRIBUTES) {
        return S_OK;
    }
    const DWORD destination_error = ::GetLastError();
    if (destination_error != ERROR_FILE_NOT_FOUND && destination_error != ERROR_PATH_NOT_FOUND) {
        return HRESULT_FROM_WIN32(destination_error);
    }

    const DWORD source_attributes = ::GetFileAttributesW(source.c_str());
    if (source_attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD source_error = ::GetLastError();
        return source_error == ERROR_FILE_NOT_FOUND || source_error == ERROR_PATH_NOT_FOUND
                   ? S_OK
                   : HRESULT_FROM_WIN32(source_error);
    }

    std::filesystem::path temporary;
    HRESULT result = make_temporary_path(destination, temporary);
    if (FAILED(result)) {
        return result;
    }
    if (!::CopyFileW(source.c_str(), temporary.c_str(), TRUE)) {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    if (::MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH)) {
        return S_OK;
    }

    const DWORD move_error = ::GetLastError();
    ::DeleteFileW(temporary.c_str());
    return move_error == ERROR_FILE_EXISTS || move_error == ERROR_ALREADY_EXISTS
               ? S_OK
               : HRESULT_FROM_WIN32(move_error);
}

HRESULT migrate_dictionary(const std::filesystem::path& source,
                           const std::filesystem::path& destination) {
    const DWORD destination_attributes = ::GetFileAttributesW(destination.c_str());
    if (destination_attributes != INVALID_FILE_ATTRIBUTES) {
        return S_OK;
    }
    const DWORD destination_error = ::GetLastError();
    if (destination_error != ERROR_FILE_NOT_FOUND && destination_error != ERROR_PATH_NOT_FOUND) {
        return HRESULT_FROM_WIN32(destination_error);
    }

    const DWORD source_attributes = ::GetFileAttributesW(source.c_str());
    if (source_attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD source_error = ::GetLastError();
        return source_error == ERROR_FILE_NOT_FOUND || source_error == ERROR_PATH_NOT_FOUND
                   ? S_OK
                   : HRESULT_FROM_WIN32(source_error);
    }

    std::filesystem::path temporary;
    HRESULT result = make_temporary_path(destination, temporary);
    if (FAILED(result)) {
        return result;
    }

    sqlite3* source_database = nullptr;
    int sqlite_result = ::sqlite3_open16(source.c_str(), &source_database);
    if (sqlite_result != SQLITE_OK) {
        if (source_database != nullptr) {
            ::sqlite3_close(source_database);
        }
        return HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
    }

    sqlite3* destination_database = nullptr;
    sqlite_result = ::sqlite3_open16(temporary.c_str(), &destination_database);
    if (sqlite_result != SQLITE_OK) {
        if (destination_database != nullptr) {
            ::sqlite3_close(destination_database);
        }
        ::sqlite3_close(source_database);
        ::DeleteFileW(temporary.c_str());
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }

    ::sqlite3_busy_timeout(source_database, 5000);
    ::sqlite3_busy_timeout(destination_database, 5000);
    sqlite3_backup* backup =
        ::sqlite3_backup_init(destination_database, "main", source_database, "main");
    if (backup == nullptr) {
        ::sqlite3_close(destination_database);
        ::sqlite3_close(source_database);
        ::DeleteFileW(temporary.c_str());
        return HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
    }

    int step_result = SQLITE_OK;
    unsigned int retries = 0;
    do {
        step_result = ::sqlite3_backup_step(backup, -1);
        if (step_result == SQLITE_BUSY || step_result == SQLITE_LOCKED) {
            ::Sleep(50);
            ++retries;
        }
    } while ((step_result == SQLITE_BUSY || step_result == SQLITE_LOCKED) && retries < 100);
    const int finish_result = ::sqlite3_backup_finish(backup);
    const int destination_close_result = ::sqlite3_close(destination_database);
    const int source_close_result = ::sqlite3_close(source_database);
    if (step_result != SQLITE_DONE || finish_result != SQLITE_OK
        || destination_close_result != SQLITE_OK || source_close_result != SQLITE_OK) {
        ::DeleteFileW(temporary.c_str());
        return HRESULT_FROM_WIN32(ERROR_FILE_INVALID);
    }

    if (::MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH)) {
        return S_OK;
    }
    const DWORD move_error = ::GetLastError();
    ::DeleteFileW(temporary.c_str());
    return move_error == ERROR_FILE_EXISTS || move_error == ERROR_ALREADY_EXISTS
               ? S_OK
               : HRESULT_FROM_WIN32(move_error);
}

}  // namespace

HRESULT get_user_data_directory(std::filesystem::path& directory) noexcept {
    const HRESULT result = get_folder_path(FOLDERID_RoamingAppData, directory);
    if (FAILED(result)) {
        return result;
    }
    try {
        return ensure_directory_exists(directory);
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    }
}

HRESULT get_legacy_user_data_directory(std::filesystem::path& directory) noexcept {
    return get_folder_path(FOLDERID_LocalAppData, directory);
}

HRESULT get_shared_data_directory(std::filesystem::path& directory) noexcept {
    return get_folder_path(FOLDERID_ProgramData, directory, 0);
}

HRESULT get_user_settings_path(std::filesystem::path& path) noexcept {
    std::filesystem::path directory;
    const HRESULT result = get_user_data_directory(directory);
    if (FAILED(result)) {
        return result;
    }
    path = directory / L"settings.ini";
    return S_OK;
}

HRESULT get_user_dictionary_path(std::filesystem::path& path) noexcept {
    std::filesystem::path directory;
    const HRESULT result = get_user_data_directory(directory);
    if (FAILED(result)) {
        return result;
    }
    path = directory / L"dictionaries" / L"user_candidates.db";
    return S_OK;
}

HRESULT get_user_layouts_directory(std::filesystem::path& directory) noexcept {
    std::filesystem::path base;
    const HRESULT result = get_user_data_directory(base);
    if (FAILED(result)) {
        return result;
    }
    directory = base / L"layouts";
    return S_OK;
}

HRESULT get_shared_assets_directory(std::filesystem::path& directory) noexcept {
    std::filesystem::path base;
    const HRESULT result = get_shared_data_directory(base);
    if (FAILED(result)) {
        return result;
    }
    directory = base / L"assets";
    return S_OK;
}

HRESULT get_shared_fonts_directory(std::filesystem::path& directory) noexcept {
    std::filesystem::path base;
    const HRESULT result = get_shared_assets_directory(base);
    if (FAILED(result)) {
        return result;
    }
    directory = base / L"fonts";
    return S_OK;
}

HRESULT get_shared_themes_directory(std::filesystem::path& directory) noexcept {
    std::filesystem::path base;
    const HRESULT result = get_shared_assets_directory(base);
    if (FAILED(result)) {
        return result;
    }
    directory = base / L"themes";
    return S_OK;
}

HRESULT get_shared_dictionaries_directory(std::filesystem::path& directory) noexcept {
    std::filesystem::path base;
    const HRESULT result = get_shared_data_directory(base);
    if (FAILED(result)) {
        return result;
    }
    directory = base / L"dictionaries";
    return S_OK;
}

HRESULT get_shared_layouts_directory(std::filesystem::path& directory) noexcept {
    std::filesystem::path base;
    const HRESULT result = get_shared_data_directory(base);
    if (FAILED(result)) {
        return result;
    }
    directory = base / L"layouts";
    return S_OK;
}

HRESULT prepare_user_data_directory(const std::filesystem::path& directory,
                                    const std::filesystem::path& legacy_directory) {
    HRESULT result = ensure_directory_exists(directory / L"dictionaries" / L"suborno_learnt_bn");
    if (FAILED(result)) {
        return result;
    }
    result = ensure_directory_exists(directory / L"layouts");
    if (FAILED(result)) {
        return result;
    }

    MigrationLock lock;
    result = lock.acquire();
    if (FAILED(result)) {
        return result;
    }

    result = migrate_settings(legacy_directory / L"settings.ini", directory / L"settings.ini");
    if (FAILED(result)) {
        return result;
    }
    return migrate_dictionary(legacy_directory / L"dictionary.sqlite3",
                              directory / L"dictionaries" / L"user_candidates.db");
}

}  // namespace suborno::platform::windows
