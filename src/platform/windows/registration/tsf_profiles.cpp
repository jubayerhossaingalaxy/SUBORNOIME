#include "tsf_profiles.hpp"

#include <msctf.h>
#include <string>
#include <wil/com.h>

namespace suborno::platform::windows::registration {
namespace {

class ComApartment final {
  public:
    HRESULT initialize() noexcept {
        const HRESULT result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        initialized_ = SUCCEEDED(result);
        return result == RPC_E_CHANGED_MODE ? S_OK : result;
    }

    ~ComApartment() {
        if (initialized_) {
            ::CoUninitialize();
        }
    }

  private:
    bool initialized_ = false;
};

HRESULT create_profiles(wil::com_ptr<ITfInputProcessorProfiles>& profiles) {
    const HRESULT result = ::CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                              nullptr,
                                              CLSCTX_INPROC_SERVER,
                                              IID_PPV_ARGS(profiles.put()));
    return result;
}

HRESULT create_category_manager(wil::com_ptr<ITfCategoryMgr>& categories) {
    return ::CoCreateInstance(
        CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(categories.put()));
}

}  // namespace

HRESULT register_tsf_profile(REFCLSID class_id,
                             REFGUID profile_id,
                             LANGID language_id,
                             std::wstring_view description,
                             std::wstring_view icon_path) {
    if (description.empty() || icon_path.empty()) {
        return E_INVALIDARG;
    }
    const std::wstring terminated_description(description);
    const std::wstring terminated_icon_path(icon_path);
    ComApartment apartment;
    HRESULT result = apartment.initialize();
    if (FAILED(result)) {
        return result;
    }

    wil::com_ptr<ITfInputProcessorProfiles> profiles;
    result = create_profiles(profiles);
    if (FAILED(result)) {
        return result;
    }

    result = profiles->Register(class_id);
    if (FAILED(result)) {
        return result;
    }

    result = profiles->AddLanguageProfile(class_id,
                                          language_id,
                                          profile_id,
                                          terminated_description.c_str(),
                                          static_cast<ULONG>(terminated_description.size()),
                                          terminated_icon_path.c_str(),
                                          static_cast<ULONG>(terminated_icon_path.size()),
                                          0);
    if (FAILED(result)) {
        profiles->Unregister(class_id);
        return result;
    }
    result = profiles->EnableLanguageProfile(class_id, language_id, profile_id, TRUE);
    if (FAILED(result)) {
        profiles->RemoveLanguageProfile(class_id, language_id, profile_id);
        profiles->Unregister(class_id);
        return result;
    }

    wil::com_ptr<ITfCategoryMgr> categories;
    result = create_category_manager(categories);
    if (SUCCEEDED(result)) {
        result = categories->RegisterCategory(class_id, GUID_TFCAT_TIP_KEYBOARD, class_id);
    }
    if (SUCCEEDED(result)) {
        result = categories->RegisterCategory(
            class_id, GUID_TFCAT_TIPCAP_UIELEMENTENABLED, class_id);
    }
    if (FAILED(result)) {
        profiles->RemoveLanguageProfile(class_id, language_id, profile_id);
        profiles->Unregister(class_id);
    }
    return result;
}

HRESULT unregister_tsf_profile(REFCLSID class_id, REFGUID, LANGID) {
    ComApartment apartment;
    HRESULT result = apartment.initialize();
    if (FAILED(result)) {
        return result;
    }

    wil::com_ptr<ITfInputProcessorProfiles> profiles;
    result = create_profiles(profiles);
    if (FAILED(result)) {
        return result;
    }

    wil::com_ptr<IEnumGUID> registered_processors;
    result = profiles->EnumInputProcessorInfo(registered_processors.put());
    if (FAILED(result)) {
        return result;
    }

    for (;;) {
        GUID registered_class{};
        ULONG fetched = 0;
        const HRESULT next_result = registered_processors->Next(1, &registered_class, &fetched);
        if (FAILED(next_result)) {
            return next_result;
        }
        if (fetched == 0) {
            return S_OK;
        }
        if (::IsEqualCLSID(registered_class, class_id)) {
            // Unregister removes the service's language profiles and categories as well.
            return profiles->Unregister(class_id);
        }
        if (next_result == S_FALSE) {
            return S_OK;
        }
    }
}

}  // namespace suborno::platform::windows::registration
