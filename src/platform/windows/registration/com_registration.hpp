#pragma once

#include <Windows.h>
#include <guiddef.h>
#include <string_view>

namespace suborno::platform::windows::registration {

HRESULT register_inproc_server(REFCLSID class_id,
                               std::wstring_view description,
                               std::wstring_view module_path);
HRESULT unregister_inproc_server(REFCLSID class_id);

}  // namespace suborno::platform::windows::registration
