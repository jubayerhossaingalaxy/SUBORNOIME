#pragma once

#include <msctf.h>
#include <string>
#include <wil/com.h>

namespace suborno::platform::windows {

class TextInsertionEditSession final : public ITfEditSession {
  public:
    static HRESULT Insert(ITfContext* context, TfClientId client_id, std::wstring text);

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD(DoEditSession)(TfEditCookie edit_cookie) override;

  private:
    TextInsertionEditSession(ITfContext* context, std::wstring text);
    ~TextInsertionEditSession() = default;

    volatile LONG reference_count_ = 1;
    wil::com_ptr<ITfContext> context_;
    std::wstring text_;
};

class TextCompositionEditSession final : public ITfEditSession {
  public:
    static HRESULT Start(ITfContext* context,
                         TfClientId client_id,
                         ITfCompositionSink* sink,
                         std::wstring text,
                         wil::com_ptr<ITfComposition>& composition);
    static HRESULT Update(ITfContext* context,
                          TfClientId client_id,
                          ITfComposition* composition,
                          std::wstring text);
    static HRESULT Finish(ITfContext* context,
                          TfClientId client_id,
                          ITfComposition* composition,
                          std::wstring text);

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD(DoEditSession)(TfEditCookie edit_cookie) override;

  private:
    TextCompositionEditSession(ITfContext* context,
                               ITfComposition* composition,
                               ITfCompositionSink* sink,
                               std::wstring text,
                               bool finish);
    ~TextCompositionEditSession() = default;

    HRESULT Request(TfClientId client_id, wil::com_ptr<ITfComposition>* composition);

    volatile LONG reference_count_ = 1;
    wil::com_ptr<ITfContext> context_;
    wil::com_ptr<ITfComposition> composition_;
    wil::com_ptr<ITfCompositionSink> sink_;
    std::wstring text_;
    bool finish_ = false;
};

}  // namespace suborno::platform::windows
