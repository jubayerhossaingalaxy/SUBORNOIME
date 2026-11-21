#pragma once

#include <Windows.h>
#include <msctf.h>
#include <inputscope.h>

namespace suborno::platform::windows {

constexpr bool is_sensitive_input_scope_type(InputScope scope) noexcept {
    switch (scope) {
    case IS_PASSWORD:
    case IS_PRIVATE:
    case IS_NUMERIC_PASSWORD:
    case IS_NUMERIC_PIN:
    case IS_ALPHANUMERIC_PIN:
    case IS_ALPHANUMERIC_PIN_SET:
        return true;
    default:
        return false;
    }
}

HRESULT query_sensitive_input_scope(ITfContext* context,
                                    TfClientId client_id,
                                    bool& sensitive) noexcept;

}  // namespace suborno::platform::windows
