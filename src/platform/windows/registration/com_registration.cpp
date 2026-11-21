#include "com_registration.hpp"

#include <iterator>
#include <shlwapi.h>
#include <string>

namespace suborno::platform::windows::registration {
namespace {

HRESULT make_class_key(REFCLSID class_id, std::wstring& key) {
    wchar_t guid[40]{};
    if (::StringFromGUID2(class_id, guid, static_cast<int>(std::size(guid))) == 0) {
        return E_INVALIDARG;
    }
    key = L"CLSID\\";
    key += guid;
    return S_OK;
}

HRESULT set_string_value(HKEY root,
                         const std::wstring& key,
                         const wchar_t* name,
                         std::wstring_view value) {
    const std::wstring terminated(value);
    HKEY handle = nullptr;
    const LONG create_result = ::RegCreateKeyExW(root,
                                                 key.c_str(),
                                                 0,
                                                 nullptr,
                                                 REG_OPTION_NON_VOLATILE,
                                                 KEY_SET_VALUE,
                                                 nullptr,
                                                 &handle,
                                                 nullptr);
    if (create_result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(create_result);
    }

    const LONG write_result =
        ::RegSetValueExW(handle,
                         name,
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(terminated.c_str()),
                         static_cast<DWORD>((terminated.size() + 1) * sizeof(wchar_t)));
    ::RegCloseKey(handle);
    return write_result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(write_result);
}

}  // namespace

HRESULT register_inproc_server(REFCLSID class_id,
                               std::wstring_view description,
                               std::wstring_view module_path) {
    if (description.empty() || module_path.empty()) {
        return E_INVALIDARG;
    }

    std::wstring class_key;
    HRESULT result = make_class_key(class_id, class_key);
    if (FAILED(result)) {
        return result;
    }

    result = set_string_value(HKEY_CLASSES_ROOT, class_key, nullptr, description);
    if (FAILED(result)) {
        ::SHDeleteKeyW(HKEY_CLASSES_ROOT, class_key.c_str());
        return result;
    }
    result =
        set_string_value(HKEY_CLASSES_ROOT, class_key + L"\\InprocServer32", nullptr, module_path);
    if (FAILED(result)) {
        ::SHDeleteKeyW(HKEY_CLASSES_ROOT, class_key.c_str());
        return result;
    }
    result = set_string_value(
        HKEY_CLASSES_ROOT, class_key + L"\\InprocServer32", L"ThreadingModel", L"Apartment");
    if (FAILED(result)) {
        ::SHDeleteKeyW(HKEY_CLASSES_ROOT, class_key.c_str());
    }
    return result;
}

HRESULT unregister_inproc_server(REFCLSID class_id) {
    std::wstring class_key;
    const HRESULT result = make_class_key(class_id, class_key);
    if (FAILED(result)) {
        return result;
    }

    const LONG delete_result = ::SHDeleteKeyW(HKEY_CLASSES_ROOT, class_key.c_str());
    if (delete_result == ERROR_FILE_NOT_FOUND || delete_result == ERROR_PATH_NOT_FOUND) {
        return S_OK;
    }
    return delete_result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(delete_result);
}

}  // namespace suborno::platform::windows::registration
