#pragma once

#include <memory>
#include <msctf.h>
#include <string>
#include <unordered_map>
#include <wil/com.h>
#include <wil/resource.h>

#include "suborno/core/engine.hpp"
#include "suborno/layout-api/input_session.hpp"

namespace suborno::platform::windows {

// TSF Text Service implementation
class SubornoTextService final : public ITfTextInputProcessor,
                                 public ITfKeyEventSink,
                                 public ITfThreadMgrEventSink,
                                 public ITfCompositionSink {
  public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // ITfTextInputProcessor
    STDMETHOD(Activate)(ITfThreadMgr* pThreadMgr, TfClientId clientId) override;
    STDMETHOD(Deactivate)() override;

    // ITfKeyEventSink
    STDMETHOD(OnKeyDown)
    (ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pbEaten) override;
    STDMETHOD(OnKeyUp)(ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pbEaten) override;
    STDMETHOD(OnTestKeyDown)
    (ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pbEaten) override;
    STDMETHOD(OnTestKeyUp)
    (ITfContext* pContext, WPARAM wParam, LPARAM lParam, BOOL* pbEaten) override;
    STDMETHOD(OnSetFocus)(BOOL fForeground) override;
    STDMETHOD(OnPreservedKey)(ITfContext* pContext, REFGUID rguid, BOOL* pbEaten) override;

    // ITfThreadMgrEventSink
    STDMETHOD(OnInitDocumentMgr)(ITfDocumentMgr* pDocMgr) override;
    STDMETHOD(OnUninitDocumentMgr)(ITfDocumentMgr* pDocMgr) override;
    STDMETHOD(OnSetFocus)(ITfDocumentMgr* pDocMgrFocus, ITfDocumentMgr* pDocMgrPrevFocus) override;
    STDMETHOD(OnPushContext)(ITfContext* pContext) override;
    STDMETHOD(OnPopContext)(ITfContext* pContext) override;
    STDMETHOD(OnCompositionTerminated)
    (TfEditCookie editCookie, ITfComposition* composition) override;

  public:
    static HRESULT CreateInstance(REFIID riid, void** ppv);

  private:
    SubornoTextService();
    ~SubornoTextService();

    // COM reference counting
    volatile LONG m_refCount = 1;

    // TSF interfaces
    wil::com_ptr<ITfThreadMgr> m_threadMgr;
    DWORD m_clientId = TF_CLIENTID_NULL;

    // Per-context composition state
    struct ContextState {
        wil::com_ptr<ITfContext> context;
        std::unique_ptr<layout::InputSession> session;
        wil::com_ptr<ITfComposition> composition;
    };
    HRESULT _CommitString(ITfContext* pContext, const std::wstring& text);
    HRESULT _UpdateComposition(ITfContext* context, ContextState& state, const std::wstring& text);
    HRESULT _FinishComposition(ITfContext* context, ContextState& state, const std::wstring& text);

    std::unordered_map<ITfContext*, ContextState> m_contexts;
    std::unique_ptr<core::Engine> m_engine;
    wil::com_ptr<ITfKeystrokeMgr> m_keystrokeMgr;
    DWORD m_threadMgrSinkCookie = TF_INVALID_COOKIE;
};

}  // namespace suborno::platform::windows
