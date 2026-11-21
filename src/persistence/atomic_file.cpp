#include "atomic_file.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <functional>
#include <limits>
#include <string>
#include <thread>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace suborno::persistence {
namespace {

std::filesystem::path make_temporary_path(const std::filesystem::path& destination) {
    static std::atomic<unsigned long long> sequence{0};
#if defined(_WIN32)
    const auto process_id = static_cast<unsigned long long>(::GetCurrentProcessId());
#else
    const auto process_id = static_cast<unsigned long long>(::getpid());
#endif
    const auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    std::filesystem::path temporary = destination;
    temporary += ".tmp." + std::to_string(process_id) + "." + std::to_string(thread_id) + "."
                 + std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
    return temporary;
}

std::error_code replace_destination(const std::filesystem::path& source,
                                    const std::filesystem::path& destination) {
#if defined(_WIN32)
    if (::MoveFileExW(source.c_str(),
                      destination.c_str(),
                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return {};
    }
    return {static_cast<int>(::GetLastError()), std::system_category()};
#else
    std::error_code error;
    std::filesystem::rename(source, destination, error);
    return error;
#endif
}

std::error_code write_temporary_file(const std::filesystem::path& path, std::string_view contents) {
#if defined(_WIN32)
    HANDLE file = ::CreateFileW(
        path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return {static_cast<int>(::GetLastError()), std::system_category()};
    }

    std::error_code error;
    std::size_t written_total = 0;
    while (written_total < contents.size()) {
        const DWORD chunk_size =
            static_cast<DWORD>(std::min<std::size_t>(contents.size() - written_total, MAXDWORD));
        DWORD written = 0;
        if (!::WriteFile(file, contents.data() + written_total, chunk_size, &written, nullptr)) {
            error = {static_cast<int>(::GetLastError()), std::system_category()};
            break;
        }
        if (written == 0) {
            error = std::make_error_code(std::errc::io_error);
            break;
        }
        written_total += written;
    }
    if (!error && !::FlushFileBuffers(file)) {
        error = {static_cast<int>(::GetLastError()), std::system_category()};
    }
    if (!::CloseHandle(file) && !error) {
        error = {static_cast<int>(::GetLastError()), std::system_category()};
    }
    if (error) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
    return error;
#else
    const int file =
        ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if (file < 0) {
        return {errno, std::generic_category()};
    }

    std::error_code error;
    std::size_t written_total = 0;
    while (written_total < contents.size()) {
        const std::size_t chunk_size =
            std::min<std::size_t>(contents.size() - written_total,
                                  static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
        const ssize_t written = ::write(file, contents.data() + written_total, chunk_size);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            error = {errno, std::generic_category()};
            break;
        }
        if (written == 0) {
            error = std::make_error_code(std::errc::io_error);
            break;
        }
        written_total += static_cast<std::size_t>(written);
    }
    if (!error && ::fsync(file) != 0) {
        error = {errno, std::generic_category()};
    }
    if (::close(file) != 0 && !error) {
        error = {errno, std::generic_category()};
    }
    if (error) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
    return error;
#endif
}

}  // namespace

std::error_code write_file_atomically(const std::filesystem::path& destination,
                                      std::string_view contents) {
    if (destination.empty()) {
        return std::make_error_code(std::errc::invalid_argument);
    }
    const std::filesystem::path temporary = make_temporary_path(destination);
    const std::error_code write_error = write_temporary_file(temporary, contents);
    if (write_error) {
        return write_error;
    }

    const std::error_code replace_error = replace_destination(temporary, destination);
    if (replace_error) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
    }
    return replace_error;
}

}  // namespace suborno::persistence
