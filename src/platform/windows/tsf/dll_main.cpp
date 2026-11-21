#include <memory>
#include <new>
#include <string>
#include <string_view>

#include "../com/module_lifetime.hpp"
#include "../registration/com_registration.hpp"
#include "../registration/tsf_profiles.hpp"
#include "suborno/platform/windows/tsf/class_factory.hpp"
#include "suborno/platform/windows/tsf/guids.hpp"
#include "suborno/platform/windows/tsf/text_service.hpp"

using namespace suborno::platform::windows;

namespace {

HINSTANCE g_moduleInstance = nullptr;

}  // namespace

_Check_return_ STDAPI DllCanUnloadNow() {
    return com::module_lifetime().can_unload();
}

_Check_return_ STDAPI DllGetClassObject(_In_ REFCLSID rclsid,
                                        _In_ REFIID riid,
                                        _Outptr_ void** ppv) {
    if (ppv == nullptr) {
        return E_POINTER;
    }
    *ppv = nullptr;

    if (rclsid == CLSID_SubornoTextService) {
        return suborno::platform::windows::SubornoClassFactory::CreateInstance(riid, ppv);
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

_Check_return_ STDAPI DllRegisterServer() {
    if (g_moduleInstance == nullptr) {
        return E_UNEXPECTED;
    }

    constexpr DWORD kModulePathCapacity = 32768;
    const std::unique_ptr<wchar_t[]> module_path(new (std::nothrow) wchar_t[kModulePathCapacity]);
    if (module_path == nullptr) {
        return E_OUTOFMEMORY;
    }
    const DWORD path_length =
        ::GetModuleFileNameW(g_moduleInstance, module_path.get(), kModulePathCapacity);
    if (path_length == 0) {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    if (path_length >= kModulePathCapacity) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    const HRESULT class_result = registration::register_inproc_server(
        CLSID_SubornoTextService, L"Suborno Bengali IME Text Service", module_path.get());
    if (FAILED(class_result)) {
        return class_result;
    }
    const HRESULT profile_result = registration::register_tsf_profile(
        CLSID_SubornoTextService,
        GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI,
        MAKELANGID(0x45, 0x02),
        L"Suborno (Bengali, Bangladesh)",
        std::wstring_view(module_path.get(), static_cast<std::size_t>(path_length)));
    if (FAILED(profile_result)) {
        registration::unregister_inproc_server(CLSID_SubornoTextService);
    }
    return profile_result;
}

_Check_return_ STDAPI DllUnregisterServer() {
    const HRESULT profile_result = registration::unregister_tsf_profile(
        CLSID_SubornoTextService, GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI, MAKELANGID(0x45, 0x02));
    const HRESULT class_result = registration::unregister_inproc_server(CLSID_SubornoTextService);
    if (FAILED(profile_result)) {
        return profile_result;
    }
    return class_result;
}

_Check_return_ STDAPI DllInstall(_In_ BOOL bInstall, _In_opt_ LPCWSTR) {
    if (bInstall) {
        return DllRegisterServer();
    }
    return DllUnregisterServer();
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        g_moduleInstance = hInstance;
        ::DisableThreadLibraryCalls(hInstance);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
