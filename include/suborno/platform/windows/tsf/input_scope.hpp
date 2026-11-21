#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <inputscope.h>
#include <msctf.h>

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

constexpr bool may_capture_text_from_input_scope(HRESULT query_result, bool sensitive) noexcept {
    return query_result == S_OK && !sensitive;
}

HRESULT query_sensitive_input_scope(ITfContext* context,
                                    TfClientId client_id,
                                    bool& sensitive) noexcept;

}  // namespace suborno::platform::windows
