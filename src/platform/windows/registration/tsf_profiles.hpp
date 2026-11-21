#pragma once

#include <Windows.h>
#include <guiddef.h>
#include <string_view>

namespace suborno::platform::windows::registration {

HRESULT register_tsf_profile(REFCLSID class_id,
                             REFGUID profile_id,
                             LANGID language_id,
                             std::wstring_view description,
                             std::wstring_view icon_path);
HRESULT unregister_tsf_profile(REFCLSID class_id, REFGUID profile_id, LANGID language_id);

}  // namespace suborno::platform::windows::registration
