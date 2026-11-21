#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/tsf/text_service.hpp"

#include <array>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <string_view>

#include "../com/module_lifetime.hpp"
#include "suborno/core/engine.hpp"
#include "suborno/layout-api/key_event.hpp"
#include "suborno/platform/windows/app_paths.hpp"
#include "suborno/platform/windows/tsf/text_edit_session.hpp"

namespace suborno::platform::windows {

namespace {

std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) {
        return L"";
    }
    if (utf8.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return {};
    }
    const int size_needed = ::MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (size_needed <= 0) {
        return L"";
    }
    std::wstring utf16(size_needed, 0);
    if (::MultiByteToWideChar(CP_UTF8,
                              MB_ERR_INVALID_CHARS,
                              utf8.c_str(),
                              static_cast<int>(utf8.size()),
                              utf16.data(),
                              size_needed)
        == 0) {
        return {};
    }
    return utf16;
}

bool is_modifier_key(WPARAM key) noexcept {
    return key == VK_SHIFT || key == VK_CONTROL || key == VK_MENU || key == VK_CAPITAL
           || key == VK_LSHIFT || key == VK_RSHIFT || key == VK_LCONTROL || key == VK_RCONTROL
           || key == VK_LMENU || key == VK_RMENU || key == VK_LWIN || key == VK_RWIN;
}

suborno::layout::KeyModifiers current_key_modifiers() noexcept {
    return {
        .shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0,
        .caps_lock = (::GetKeyState(VK_CAPITAL) & 0x0001) != 0,
        .num_lock = (::GetKeyState(VK_NUMLOCK) & 0x0001) != 0,
        .control = (::GetKeyState(VK_CONTROL) & 0x8000) != 0,
        .alt = (::GetKeyState(VK_MENU) & 0x8000) != 0,
        .windows = (::GetKeyState(VK_LWIN) & 0x8000) != 0 || (::GetKeyState(VK_RWIN) & 0x8000) != 0,
    };
}

std::optional<suborno::layout::PhysicalKey> physical_key_from_virtual_key(
    WPARAM virtual_key) noexcept {
    if (virtual_key >= 'A' && virtual_key <= 'Z') {
        const auto key = static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(suborno::layout::PhysicalKey::letter_a)
            + static_cast<std::uint8_t>(virtual_key - 'A'));
        return static_cast<suborno::layout::PhysicalKey>(key);
    }
    if (virtual_key >= '0' && virtual_key <= '9') {
        return static_cast<suborno::layout::PhysicalKey>(static_cast<std::uint8_t>(virtual_key));
    }
    if (virtual_key >= VK_NUMPAD0 && virtual_key <= VK_NUMPAD9) {
        const auto key = static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(suborno::layout::PhysicalKey::numpad_0)
            + static_cast<std::uint8_t>(virtual_key - VK_NUMPAD0));
        return static_cast<suborno::layout::PhysicalKey>(key);
    }

    using suborno::layout::PhysicalKey;
    switch (virtual_key) {
    case VK_OEM_1:
        return PhysicalKey::semicolon;
    case VK_OEM_PLUS:
        return PhysicalKey::equal;
    case VK_OEM_COMMA:
        return PhysicalKey::comma;
    case VK_OEM_MINUS:
        return PhysicalKey::minus;
    case VK_OEM_PERIOD:
        return PhysicalKey::period;
    case VK_OEM_2:
        return PhysicalKey::slash;
    case VK_OEM_3:
        return PhysicalKey::grave;
    case VK_OEM_4:
        return PhysicalKey::left_bracket;
    case VK_OEM_5:
        return PhysicalKey::backslash;
    case VK_OEM_6:
        return PhysicalKey::right_bracket;
    case VK_OEM_7:
        return PhysicalKey::apostrophe;
    case VK_SPACE:
        return PhysicalKey::space;
    default:
        return std::nullopt;
    }
}

bool logical_text_from_keyboard_layout(UINT virtual_key,
                                           UINT scan_code,
                                           bool ignore_caps_lock,
                                           std::string& output) {
    std::array<BYTE, 256> keyboard_state{};
    if (::GetKeyboardState(keyboard_state.data()) == FALSE) {
        return false;
    }
    if (ignore_caps_lock) {
        keyboard_state[VK_CAPITAL] = static_cast<BYTE>(keyboard_state[VK_CAPITAL] & 0xFEU);
    }

    std::array<wchar_t, 8> translated{};
    const HKL keyboard_layout = ::GetKeyboardLayout(0);
    const int translated_count = ::ToUnicodeEx(virtual_key,
                                                scan_code,
                                                keyboard_state.data(),
                                                translated.data(),
                                                static_cast<int>(translated.size()),
                                                0x4,
                                                keyboard_layout);
    if (translated_count <= 0) {
        return true;
    }

    const int required = ::WideCharToMultiByte(CP_UTF8,
                                               WC_ERR_INVALID_CHARS,
                                               translated.data(),
                                               translated_count,
                                               nullptr,
                                               0,
                                               nullptr,
                                               nullptr);
    if (required <= 0) {
        return false;
    }

    std::string converted(static_cast<std::size_t>(required), '\0');
    if (::WideCharToMultiByte(CP_UTF8,
                              WC_ERR_INVALID_CHARS,
                              translated.data(),
                              translated_count,
                              converted.data(),
                              required,
                              nullptr,
                              nullptr)
        != required) {
        return false;
    }
    output = std::move(converted);
    return true;
}

std::optional<suborno::layout::KeyEvent> key_event_from_virtual_key(
    WPARAM virtual_key, LPARAM lparam, bool ignore_caps_lock) {
    const auto physical_key = physical_key_from_virtual_key(virtual_key);
    if (!physical_key.has_value()) {
        return std::nullopt;
    }

    const auto message_data = suborno::layout::decode_key_message_lparam(
        static_cast<std::uint32_t>(static_cast<ULONG_PTR>(lparam)));
    suborno::layout::KeyEvent event{
        .key = *physical_key,
        .modifiers = current_key_modifiers(),
        .scan_code = message_data.scan_code,
        .extended_key = message_data.extended_key,
        .repeat_count = message_data.repeat_count == 0 ? 1 : message_data.repeat_count,
        .previous_key_state = message_data.previous_key_state,
        .key_down = message_data.key_down,
    };
    if (event.key_down && !event.modifiers.control && !event.modifiers.alt
        && !event.modifiers.windows) {
        event.logical_text_available = logical_text_from_keyboard_layout(
            static_cast<UINT>(virtual_key),
            message_data.scan_code,
            ignore_caps_lock,
            event.logical_text_utf8);
    }
    return event;
}

void apply_input_settings(std::optional<suborno::layout::KeyEvent>& event,
                          const suborno::settings::EngineSettings& settings) noexcept {
    if (!event.has_value()) {
        return;
    }

    if (settings.input.ignore_caps_lock_state_in_bangla) {
        event->modifiers.caps_lock = false;
    }
    if (!settings.input.map_numpad_to_bangla_numbers
        && suborno::layout::is_numpad_key(event->key)) {
        event.reset();
    }
}

bool has_text_modifier_chord() noexcept {
    const auto modifiers = current_key_modifiers();
    return modifiers.control || modifiers.alt || modifiers.windows;
}

std::optional<std::size_t> candidate_index_from_virtual_key(WPARAM virtual_key) noexcept {
    const auto physical_key = physical_key_from_virtual_key(virtual_key);
    if (!physical_key.has_value()) {
        return std::nullopt;
    }
    const suborno::layout::KeyEvent key_event{*physical_key, current_key_modifiers()};
    const auto character = suborno::layout::key_event_to_ascii(key_event);
    if (!character.has_value() || *character < '1' || *character > '9') {
        return std::nullopt;
    }
    return static_cast<std::size_t>(*character - '1');
}

}  // namespace

SubornoTextService::SubornoTextService() {
    com::module_lifetime().object_created();
}

SubornoTextService::~SubornoTextService() {
    com::module_lifetime().object_destroyed();
}

STDMETHODIMP SubornoTextService::QueryInterface(REFIID riid, void** ppvObj) {
    if (ppvObj == nullptr) {
        return E_POINTER;
    }
    *ppvObj = nullptr;

    if (riid == IID_IUnknown || riid == IID_ITfTextInputProcessor) {
        *ppvObj = static_cast<ITfTextInputProcessor*>(this);
    } else if (riid == IID_ITfKeyEventSink) {
        *ppvObj = static_cast<ITfKeyEventSink*>(this);
    } else if (riid == IID_ITfThreadMgrEventSink) {
        *ppvObj = static_cast<ITfThreadMgrEventSink*>(this);
    } else if (riid == IID_ITfCompositionSink) {
        *ppvObj = static_cast<ITfCompositionSink*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) SubornoTextService::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&m_refCount));
}

STDMETHODIMP_(ULONG) SubornoTextService::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&m_refCount));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP SubornoTextService::Activate(ITfThreadMgr* pThreadMgr, TfClientId clientId) {
    if (pThreadMgr == nullptr || clientId == TF_CLIENTID_NULL) {
        return E_INVALIDARG;
    }

    m_threadMgr = pThreadMgr;
    m_clientId = clientId;
    try {
        std::filesystem::path user_data;
        std::filesystem::path legacy_user_data;
        HRESULT path_result = get_user_data_directory(user_data);
        if (SUCCEEDED(path_result)) {
            path_result = get_legacy_user_data_directory(legacy_user_data);
        }
        if (SUCCEEDED(path_result)) {
            path_result = prepare_user_data_directory(user_data, legacy_user_data);
        }
        if (FAILED(path_result)) {
            m_threadMgr.reset();
            m_clientId = TF_CLIENTID_NULL;
            return path_result;
        }
        core::EngineConfig config;
        config.settings_path = user_data / L"settings.ini";
        config.dictionary_path = user_data / L"dictionaries" / L"user_candidates.db";
        m_engine = std::make_unique<core::Engine>(config);
    } catch (const std::bad_alloc&) {
        m_threadMgr.reset();
        m_clientId = TF_CLIENTID_NULL;
        return E_OUTOFMEMORY;
    } catch (...) {
        m_threadMgr.reset();
        m_clientId = TF_CLIENTID_NULL;
        return E_FAIL;
    }

    wil::com_ptr<ITfSource> source;
    HRESULT hr = m_threadMgr->QueryInterface(IID_PPV_ARGS(source.put()));
    if (FAILED(hr)) {
        m_engine.reset();
        m_threadMgr.reset();
        m_clientId = TF_CLIENTID_NULL;
        return hr;
    }
    hr = source->AdviseSink(IID_ITfThreadMgrEventSink,
                            static_cast<ITfThreadMgrEventSink*>(this),
                            &m_threadMgrSinkCookie);
    if (FAILED(hr)) {
        m_engine.reset();
        m_threadMgr.reset();
        m_clientId = TF_CLIENTID_NULL;
        return hr;
    }

    hr = m_threadMgr->QueryInterface(IID_PPV_ARGS(m_keystrokeMgr.put()));
    if (SUCCEEDED(hr)) {
        hr = m_keystrokeMgr->AdviseKeyEventSink(m_clientId, this, TRUE);
    }
    if (FAILED(hr)) {
        source->UnadviseSink(m_threadMgrSinkCookie);
        m_threadMgrSinkCookie = TF_INVALID_COOKIE;
        m_keystrokeMgr.reset();
        m_engine.reset();
        m_threadMgr.reset();
        m_clientId = TF_CLIENTID_NULL;
        return hr;
    }
    return S_OK;
}

STDMETHODIMP SubornoTextService::Deactivate() {
    HRESULT first_error = S_OK;
    try {
        for (auto& [context, state] : m_contexts) {
            if (state.session != nullptr && state.composition != nullptr) {
                std::string text_utf8;
                text_utf8 = state.session->commit().text_utf8;
                const std::wstring text = utf8_to_utf16(text_utf8);
                if (!text_utf8.empty() && text.empty() && SUCCEEDED(first_error)) {
                    first_error = E_INVALIDARG;
                }
                const HRESULT finish_result = _FinishComposition(context, state, text);
                if (FAILED(finish_result) && SUCCEEDED(first_error)) {
                    first_error = finish_result;
                }
            }
        }
    } catch (const std::bad_alloc&) {
        first_error = E_OUTOFMEMORY;
    } catch (...) {
        first_error = E_FAIL;
    }
    if (m_keystrokeMgr != nullptr && m_clientId != TF_CLIENTID_NULL) {
        const HRESULT hr = m_keystrokeMgr->UnadviseKeyEventSink(m_clientId);
        if (FAILED(hr) && SUCCEEDED(first_error)) {
            first_error = hr;
        }
    }
    m_keystrokeMgr.reset();
    if (m_threadMgr != nullptr && m_threadMgrSinkCookie != TF_INVALID_COOKIE) {
        wil::com_ptr<ITfSource> source;
        const HRESULT query_result = m_threadMgr->QueryInterface(IID_PPV_ARGS(source.put()));
        const HRESULT hr =
            FAILED(query_result) ? query_result : source->UnadviseSink(m_threadMgrSinkCookie);
        if (FAILED(hr) && SUCCEEDED(first_error)) {
            first_error = hr;
        }
    }
    m_threadMgrSinkCookie = TF_INVALID_COOKIE;
    m_threadMgr.reset();
    m_clientId = TF_CLIENTID_NULL;
    m_contexts.clear();
    m_engine.reset();
    return first_error;
}

STDMETHODIMP SubornoTextService::OnInitDocumentMgr(ITfDocumentMgr*) {
    return S_OK;
}

STDMETHODIMP SubornoTextService::OnUninitDocumentMgr(ITfDocumentMgr*) {
    return S_OK;
}

STDMETHODIMP SubornoTextService::OnTestKeyDown(ITfContext* pContext,
                                               WPARAM wParam,
                                               LPARAM lParam,
                                               BOOL* pbEaten) {
    if (pbEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pbEaten = FALSE;
    if (pContext == nullptr || m_engine == nullptr || is_modifier_key(wParam)) {
        return S_OK;
    }
    if (has_text_modifier_chord()) {
        return S_OK;
    }

    try {
        const auto found = m_contexts.find(pContext);
        const bool composing = found != m_contexts.end() && found->second.session != nullptr
                               && !found->second.session->snapshot().raw_input_utf8.empty();
        const auto& input_settings = m_engine->settings().input;
        auto key_event = key_event_from_virtual_key(
            wParam, lParam, input_settings.ignore_caps_lock_state_in_bangla);
        apply_input_settings(key_event, m_engine->settings());
        const bool printable =
            key_event.has_value()
            && (key_event->logical_text_available
                    ? !key_event->logical_text_utf8.empty()
                    : suborno::layout::key_event_to_ascii(*key_event).has_value());
        const bool starts_phonetic_input =
            key_event.has_value()
            && (key_event->logical_text_available
                    ? key_event->logical_text_utf8.size() == 1
                          && ((key_event->logical_text_utf8.front() >= 'A'
                               && key_event->logical_text_utf8.front() <= 'Z')
                              || (key_event->logical_text_utf8.front() >= 'a'
                                  && key_event->logical_text_utf8.front() <= 'z'))
                    : wParam >= 'A' && wParam <= 'Z');
        const bool fixed_or_compositional =
            m_engine->active_layout_kind() != suborno::layout::LayoutKind::phonetic;
        const bool starts_composition = fixed_or_compositional ? printable : starts_phonetic_input;
        *pbEaten = (composing || starts_composition) && (composing || wParam != VK_SPACE);
        return S_OK;
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    } catch (...) {
        return E_FAIL;
    }
}

STDMETHODIMP SubornoTextService::OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pbEaten) {
    if (pbEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pbEaten = FALSE;
    return S_OK;
}

STDMETHODIMP SubornoTextService::OnKeyDown(ITfContext* pContext,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL* pbEaten) {
    if (pbEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pbEaten = FALSE;
    if (pContext == nullptr || m_engine == nullptr) {
        return E_INVALIDARG;
    }
    if (is_modifier_key(wParam) || has_text_modifier_chord()) {
        return S_OK;
    }

    try {
        auto [found, inserted] = m_contexts.try_emplace(pContext);
        ContextState& state = found->second;
        if (inserted) {
            state.context = pContext;
        }
        if (state.session == nullptr) {
            state.session = m_engine->create_session();
        }
        if (state.session == nullptr) {
            return E_UNEXPECTED;
        }

        const auto snapshot = state.session->snapshot();
        const bool composing = !snapshot.raw_input_utf8.empty();
        if (!composing && wParam == VK_SPACE) {
            return S_OK;
        }

        if (composing && wParam == VK_BACK) {
            state.session->backspace();
            const auto updated = state.session->snapshot();
            const HRESULT hr =
                updated.raw_input_utf8.empty()
                    ? _FinishComposition(pContext, state, {})
                    : _UpdateComposition(pContext, state, utf8_to_utf16(updated.preedit_utf8));
            if (SUCCEEDED(hr)) {
                *pbEaten = TRUE;
            }
            return hr;
        }
        if (composing && wParam == VK_ESCAPE) {
            state.session->reset();
            const HRESULT hr = _FinishComposition(pContext, state, {});
            if (SUCCEEDED(hr)) {
                *pbEaten = TRUE;
            }
            return hr;
        }
        if (composing) {
            const auto candidate_index =
                m_engine->settings().input.select_candidate_using_number_keys
                    ? candidate_index_from_virtual_key(wParam)
                    : std::nullopt;
            if (candidate_index.has_value() && *candidate_index < snapshot.candidates.size()) {
                const auto committed = state.session->commit_candidate(*candidate_index);
                const std::wstring text = utf8_to_utf16(committed.text_utf8);
                if (!committed.text_utf8.empty() && text.empty()) {
                    return E_INVALIDARG;
                }
                const HRESULT hr = _FinishComposition(pContext, state, text);
                if (SUCCEEDED(hr)) {
                    *pbEaten = TRUE;
                }
                return hr;
            }
        }
        if (composing && wParam == VK_SPACE) {
            const auto committed = state.session->commit();
            const std::wstring text = utf8_to_utf16(committed.text_utf8);
            if (!committed.text_utf8.empty() && text.empty()) {
                return E_INVALIDARG;
            }
            std::wstring with_space = text;
            with_space.push_back(L' ');
            const HRESULT hr = _FinishComposition(pContext, state, with_space);
            if (SUCCEEDED(hr)) {
                *pbEaten = TRUE;
            }
            return hr;
        }

        const auto& input_settings = m_engine->settings().input;
        auto key_event = key_event_from_virtual_key(
            wParam, lParam, input_settings.ignore_caps_lock_state_in_bangla);
        apply_input_settings(key_event, m_engine->settings());
        const bool starts_phonetic_input =
            key_event.has_value() && !key_event->logical_text_utf8.empty()
            && key_event->logical_text_utf8.size() == 1
            && ((key_event->logical_text_utf8.front() >= 'A'
                 && key_event->logical_text_utf8.front() <= 'Z')
                || (key_event->logical_text_utf8.front() >= 'a'
                    && key_event->logical_text_utf8.front() <= 'z'));
        const bool fixed_or_compositional =
            m_engine->active_layout_kind() != layout::LayoutKind::phonetic;
        const bool should_append_key =
            key_event.has_value() && (composing || fixed_or_compositional || starts_phonetic_input);
        if (should_append_key) {
            const layout::InputStatus input_status = state.session->append_key(*key_event);
            if (input_status == layout::InputStatus::invalid_utf8
                || input_status == layout::InputStatus::input_limit
                || input_status == layout::InputStatus::empty) {
                return S_OK;
            }
            const auto updated = state.session->snapshot();
            const std::wstring preedit = utf8_to_utf16(updated.preedit_utf8);
            if (preedit.empty()) {
                return E_FAIL;
            }
            const HRESULT hr = _UpdateComposition(pContext, state, preedit);
            if (SUCCEEDED(hr)) {
                *pbEaten = TRUE;
            }
            return hr;
        }

        if (!composing) {
            return S_OK;
        }

        const auto committed = state.session->commit();
        const std::wstring text = utf8_to_utf16(committed.text_utf8);
        if (!committed.text_utf8.empty() && text.empty()) {
            return E_INVALIDARG;
        }
        const HRESULT hr = _FinishComposition(pContext, state, text);
        if (FAILED(hr)) {
            return hr;
        }
        return S_OK;
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    } catch (...) {
        return E_FAIL;
    }
}

STDMETHODIMP SubornoTextService::OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pbEaten) {
    if (pbEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pbEaten = FALSE;
    return S_OK;
}

STDMETHODIMP SubornoTextService::OnSetFocus(BOOL) {
    return S_OK;
}

STDMETHODIMP SubornoTextService::OnPreservedKey(ITfContext*, REFGUID, BOOL* pbEaten) {
    if (pbEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pbEaten = FALSE;
    return S_OK;
}

STDMETHODIMP SubornoTextService::OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*) {
    return S_OK;
}

STDMETHODIMP SubornoTextService::OnPushContext(ITfContext* pContext) {
    if (pContext == nullptr || m_engine == nullptr) {
        return E_INVALIDARG;
    }
    try {
        auto [found, inserted] = m_contexts.try_emplace(pContext);
        if (inserted) {
            found->second.context = pContext;
        }
        if (found->second.session == nullptr) {
            found->second.session = m_engine->create_session();
        }
        return found->second.session == nullptr ? E_UNEXPECTED : S_OK;
    } catch (const std::bad_alloc&) {
        return E_OUTOFMEMORY;
    } catch (...) {
        return E_FAIL;
    }
}

STDMETHODIMP SubornoTextService::OnPopContext(ITfContext* pContext) {
    const auto found = m_contexts.find(pContext);
    if (found == m_contexts.end()) {
        return S_OK;
    }
    HRESULT result = S_OK;
    try {
        if (found->second.session != nullptr && found->second.composition != nullptr) {
            const auto committed = found->second.session->commit();
            const std::wstring text = utf8_to_utf16(committed.text_utf8);
            if (!committed.text_utf8.empty() && text.empty()) {
                result = E_INVALIDARG;
            } else {
                result = _FinishComposition(pContext, found->second, text);
            }
        }
    } catch (const std::bad_alloc&) {
        result = E_OUTOFMEMORY;
    } catch (...) {
        result = E_FAIL;
    }
    m_contexts.erase(found);
    return result;
}

STDMETHODIMP SubornoTextService::OnCompositionTerminated(TfEditCookie,
                                                         ITfComposition* composition) {
    if (composition == nullptr) {
        return E_INVALIDARG;
    }
    for (auto& [context, state] : m_contexts) {
        static_cast<void>(context);
        if (state.composition.get() == composition) {
            state.composition.reset();
            if (state.session != nullptr) {
                state.session->reset();
            }
            break;
        }
    }
    return S_OK;
}

HRESULT SubornoTextService::_UpdateComposition(ITfContext* context,
                                               ContextState& state,
                                               const std::wstring& text) {
    if (state.composition == nullptr) {
        return TextCompositionEditSession::Start(
            context, m_clientId, this, text, state.composition);
    }
    return TextCompositionEditSession::Update(context, m_clientId, state.composition.get(), text);
}

HRESULT SubornoTextService::_FinishComposition(ITfContext* context,
                                               ContextState& state,
                                               const std::wstring& text) {
    HRESULT result = S_OK;
    if (state.composition != nullptr) {
        result =
            TextCompositionEditSession::Finish(context, m_clientId, state.composition.get(), text);
        if (SUCCEEDED(result)) {
            state.composition.reset();
        }
    } else if (!text.empty()) {
        result = _CommitString(context, text);
    }
    if (SUCCEEDED(result) && state.session != nullptr) {
        state.session->reset();
    }
    return result;
}

HRESULT SubornoTextService::_CommitString(ITfContext* pContext, const std::wstring& text) {
    if (pContext == nullptr) {
        return E_INVALIDARG;
    }
    if (text.empty()) {
        return S_OK;
    }
    return TextInsertionEditSession::Insert(pContext, m_clientId, text);
}

HRESULT SubornoTextService::CreateInstance(REFIID riid, void** ppv) {
    if (ppv == nullptr) {
        return E_POINTER;
    }
    *ppv = nullptr;

    SubornoTextService* service = new (std::nothrow) SubornoTextService();
    if (service == nullptr) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = service->QueryInterface(riid, ppv);
    service->Release();
    return hr;
}

}  // namespace suborno::platform::windows
