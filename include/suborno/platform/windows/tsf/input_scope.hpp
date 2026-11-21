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

constexpr bool may_commit_composition_from_input_scope(HRESULT query_result,
                                                       bool sensitive,
                                                       bool previously_blocked) noexcept {
    return !previously_blocked && may_capture_text_from_input_scope(query_result, sensitive);
}

enum class InputScopeCaptureDecision {
    allow_capture,
    pass_through,
    cancel_composition,
};

constexpr InputScopeCaptureDecision decide_input_scope_capture(HRESULT query_result,
                                                               bool sensitive,
                                                               bool composing) noexcept {
    if (may_capture_text_from_input_scope(query_result, sensitive)) {
        return InputScopeCaptureDecision::allow_capture;
    }
    return composing ? InputScopeCaptureDecision::cancel_composition
                     : InputScopeCaptureDecision::pass_through;
}

HRESULT query_sensitive_input_scope(ITfContext* context,
                                    TfClientId client_id,
                                    bool& sensitive) noexcept;

}  // namespace suborno::platform::windows
