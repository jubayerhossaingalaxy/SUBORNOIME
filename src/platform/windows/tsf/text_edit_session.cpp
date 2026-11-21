#include "suborno/platform/windows/tsf/text_edit_session.hpp"

#include <climits>
#include <memory>
#include <new>
#include <utility>

namespace suborno::platform::windows {
namespace {

HRESULT request_session(ITfContext* context, TfClientId client_id, ITfEditSession* session) {
    HRESULT session_result = E_FAIL;
    const HRESULT request_result = context->RequestEditSession(
        client_id, session, TF_ES_SYNC | TF_ES_READWRITE, &session_result);
    return FAILED(request_result) ? request_result : session_result;
}

HRESULT set_composition_text(TfEditCookie edit_cookie,
                             ITfContext* context,
                             ITfComposition* composition,
                             const std::wstring& text) {
    wil::com_ptr<ITfRange> range;
    HRESULT result = composition->GetRange(range.put());
    if (FAILED(result)) {
        return result;
    }

    result = range->SetText(edit_cookie, 0, text.c_str(), static_cast<LONG>(text.size()));
    if (FAILED(result)) {
        return result;
    }

    result = range->Collapse(edit_cookie, TF_ANCHOR_END);
    if (FAILED(result)) {
        return result;
    }

    TF_SELECTION selection{};
    selection.range = range.get();
    selection.style.ase = TF_AE_END;
    selection.style.fInterimChar = FALSE;
    return context->SetSelection(edit_cookie, 1, &selection);
}

}  // namespace

TextInsertionEditSession::TextInsertionEditSession(ITfContext* context, std::wstring text)
    : context_(context),
      text_(std::move(text)) {}

HRESULT TextInsertionEditSession::Insert(ITfContext* context,
                                         TfClientId client_id,
                                         std::wstring text) {
    if (context == nullptr || text.size() > static_cast<std::size_t>(LONG_MAX)) {
        return E_INVALIDARG;
    }

    auto* session = new (std::nothrow) TextInsertionEditSession(context, std::move(text));
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = request_session(context, client_id, session);
    session->Release();
    return result;
}

STDMETHODIMP TextInsertionEditSession::QueryInterface(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    *object = nullptr;

    if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
        *object = static_cast<ITfEditSession*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) TextInsertionEditSession::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) TextInsertionEditSession::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&reference_count_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP TextInsertionEditSession::DoEditSession(TfEditCookie edit_cookie) {
    wil::com_ptr<ITfInsertAtSelection> insertion;
    const HRESULT query_result = context_->QueryInterface(IID_PPV_ARGS(insertion.put()));
    if (FAILED(query_result)) {
        return query_result;
    }

    wil::com_ptr<ITfRange> inserted_range;
    return insertion->InsertTextAtSelection(edit_cookie,
                                            TF_IAS_NOQUERY,
                                            text_.c_str(),
                                            static_cast<LONG>(text_.size()),
                                            inserted_range.put());
}

TextCompositionEditSession::TextCompositionEditSession(ITfContext* context,
                                                       ITfComposition* composition,
                                                       ITfCompositionSink* sink,
                                                       std::wstring text,
                                                       bool finish)
    : context_(context),
      composition_(composition),
      sink_(sink),
      text_(std::move(text)),
      finish_(finish) {}

HRESULT TextCompositionEditSession::Start(ITfContext* context,
                                          TfClientId client_id,
                                          ITfCompositionSink* sink,
                                          std::wstring text,
                                          wil::com_ptr<ITfComposition>& composition) {
    if (context == nullptr || sink == nullptr || text.size() > static_cast<std::size_t>(LONG_MAX)) {
        return E_INVALIDARG;
    }
    auto* session = new (std::nothrow)
        TextCompositionEditSession(context, nullptr, sink, std::move(text), false);
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = session->Request(client_id, std::addressof(composition));
    session->Release();
    return result;
}

HRESULT TextCompositionEditSession::Update(ITfContext* context,
                                           TfClientId client_id,
                                           ITfComposition* composition,
                                           std::wstring text) {
    if (context == nullptr || composition == nullptr
        || text.size() > static_cast<std::size_t>(LONG_MAX)) {
        return E_INVALIDARG;
    }
    auto* session = new (std::nothrow)
        TextCompositionEditSession(context, composition, nullptr, std::move(text), false);
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = session->Request(client_id, nullptr);
    session->Release();
    return result;
}

HRESULT TextCompositionEditSession::Finish(ITfContext* context,
                                           TfClientId client_id,
                                           ITfComposition* composition,
                                           std::wstring text) {
    if (context == nullptr || composition == nullptr
        || text.size() > static_cast<std::size_t>(LONG_MAX)) {
        return E_INVALIDARG;
    }
    auto* session = new (std::nothrow)
        TextCompositionEditSession(context, composition, nullptr, std::move(text), true);
    if (session == nullptr) {
        return E_OUTOFMEMORY;
    }
    const HRESULT result = session->Request(client_id, nullptr);
    session->Release();
    return result;
}

HRESULT TextCompositionEditSession::Request(TfClientId client_id,
                                            wil::com_ptr<ITfComposition>* composition) {
    HRESULT result = request_session(context_.get(), client_id, this);
    if (SUCCEEDED(result) && composition != nullptr) {
        *composition = composition_;
    }
    return result;
}

STDMETHODIMP TextCompositionEditSession::QueryInterface(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    *object = nullptr;

    if (riid == IID_IUnknown || riid == IID_ITfEditSession) {
        *object = static_cast<ITfEditSession*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) TextCompositionEditSession::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) TextCompositionEditSession::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&reference_count_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP TextCompositionEditSession::DoEditSession(TfEditCookie edit_cookie) {
    if (composition_ == nullptr) {
        TF_SELECTION selection{};
        ULONG fetched = 0;
        HRESULT result =
            context_->GetSelection(edit_cookie, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
        if (FAILED(result)) {
            return result;
        }
        if (fetched != 1 || selection.range == nullptr) {
            if (selection.range != nullptr) {
                selection.range->Release();
            }
            return E_UNEXPECTED;
        }

        wil::com_ptr<ITfContextComposition> context_composition;
        result = context_->QueryInterface(IID_PPV_ARGS(context_composition.put()));
        if (SUCCEEDED(result)) {
            result = context_composition->StartComposition(
                edit_cookie, selection.range, sink_.get(), composition_.put());
        }
        selection.range->Release();
        if (FAILED(result)) {
            return result;
        }
    }

    HRESULT result = set_composition_text(edit_cookie, context_.get(), composition_.get(), text_);
    if (FAILED(result)) {
        if (!finish_) {
            composition_->EndComposition(edit_cookie);
        }
        return result;
    }
    if (finish_) {
        result = composition_->EndComposition(edit_cookie);
        if (SUCCEEDED(result)) {
            composition_.reset();
        }
    }
    return result;
}

}  // namespace suborno::platform::windows
