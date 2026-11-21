#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/tsf/text_service.hpp"

#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>

#include "../com/module_lifetime.hpp"
#include "suborno/core/engine.hpp"
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
           || key == VK_LMENU || key == VK_RMENU;
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
                                               LPARAM,
                                               BOOL* pbEaten) {
    if (pbEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pbEaten = FALSE;
    if (pContext == nullptr || m_engine == nullptr || is_modifier_key(wParam)) {
        return S_OK;
    }
    try {
        const auto found = m_contexts.find(pContext);
        const bool composing = found != m_contexts.end() && found->second.session != nullptr
                               && !found->second.session->snapshot().raw_input_utf8.empty();
        *pbEaten = (wParam >= 'A' && wParam <= 'Z') || composing;
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
                                           LPARAM,
                                           BOOL* pbEaten) {
    if (pbEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pbEaten = FALSE;
    if (pContext == nullptr || m_engine == nullptr) {
        return E_INVALIDARG;
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

        if (wParam >= 'A' && wParam <= 'Z') {
            const bool shifted = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
            const bool caps_locked = (::GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            char character = static_cast<char>(wParam);
            if (shifted != caps_locked) {
                character = static_cast<char>(character + ('a' - 'A'));
            }
            const layouts::InputStatus input_status =
                state.session->append_utf8(std::string_view(&character, 1));
            if (input_status == layouts::InputStatus::invalid_utf8
                || input_status == layouts::InputStatus::input_limit) {
                return S_OK;
            }
            const std::wstring preedit = utf8_to_utf16(state.session->snapshot().preedit_utf8);
            if (preedit.empty()) {
                return E_FAIL;
            }
            const HRESULT hr = _UpdateComposition(pContext, state, preedit);
            if (SUCCEEDED(hr)) {
                *pbEaten = TRUE;
            }
            return hr;
        }

        const auto snapshot = state.session->snapshot();
        if (snapshot.raw_input_utf8.empty()) {
            return S_OK;
        }

        if (wParam == VK_BACK) {
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
        if (wParam == VK_ESCAPE) {
            state.session->reset();
            const HRESULT hr = _FinishComposition(pContext, state, {});
            if (SUCCEEDED(hr)) {
                *pbEaten = TRUE;
            }
            return hr;
        }
        if (wParam >= '1' && wParam <= '9') {
            const std::size_t candidate_index = static_cast<std::size_t>(wParam - '1');
            const auto candidates = snapshot.candidates;
            if (candidate_index >= candidates.size()) {
                return S_OK;
            }
            const auto committed = state.session->commit_candidate(candidate_index);
            const std::wstring text = utf8_to_utf16(committed.text_utf8);
            if (text.empty()) {
                return E_FAIL;
            }
            const HRESULT hr = _FinishComposition(pContext, state, text);
            if (SUCCEEDED(hr)) {
                *pbEaten = TRUE;
            }
            return hr;
        }

        std::wstring committed;
        if (wParam == VK_SPACE) {
            const auto result = state.session->commit();
            committed = utf8_to_utf16(result.text_utf8);
            if (committed.empty()) {
                return E_FAIL;
            }
            committed.push_back(L' ');
        } else {
            committed = utf8_to_utf16(state.session->commit().text_utf8);
        }
        const HRESULT hr = _FinishComposition(pContext, state, committed);
        if (FAILED(hr)) {
            return hr;
        }
        *pbEaten = wParam == VK_SPACE;
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
