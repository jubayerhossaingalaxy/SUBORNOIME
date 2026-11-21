#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <cstddef>
#include <functional>
#include <msctf.h>
#include <string>
#include <string_view>
#include <vector>
#include <wil/com.h>

#include "suborno/layout-api/candidate.hpp"

namespace suborno::platform::windows {

inline constexpr GUID kSubornoCandidateListUIElementGuid{
    0x0b8f6f43,
    0x8858,
    0x4e58,
    {0x9a, 0x2d, 0x4a, 0x6b, 0x31, 0x9d, 0x2c, 0x11},
};

class CandidateListUIElement final : public ITfCandidateListUIElement,
                                     public ITfCandidateListUIElementBehavior {
  public:
    using FinalizeCallback = std::function<HRESULT(UINT)>;
    using AbortCallback = std::function<HRESULT()>;

    CandidateListUIElement(ITfContext* context,
                           const std::vector<layout::Candidate>& candidates,
                           std::size_t selected_index,
                           FinalizeCallback finalize_callback,
                           AbortCallback abort_callback);

    STDMETHOD(QueryInterface)(REFIID riid, void** object) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    STDMETHOD(GetDescription)(BSTR* description) override;
    STDMETHOD(GetGUID)(GUID* guid) override;
    STDMETHOD(Show)(BOOL show) override;
    STDMETHOD(IsShown)(BOOL* show) override;

    STDMETHOD(GetUpdatedFlags)(DWORD* flags) override;
    STDMETHOD(GetDocumentMgr)(ITfDocumentMgr** document_manager) override;
    STDMETHOD(GetCount)(UINT* count) override;
    STDMETHOD(GetSelection)(UINT* index) override;
    STDMETHOD(GetString)(UINT index, BSTR* text) override;
    STDMETHOD(GetPageIndex)(UINT* index, UINT size, UINT* page_count) override;
    STDMETHOD(SetPageIndex)(UINT* index, UINT page_count) override;
    STDMETHOD(GetCurrentPage)(UINT* page) override;

    STDMETHOD(SetSelection)(UINT index) override;
    STDMETHOD(Finalize)() override;
    STDMETHOD(Abort)() override;

    void Update(const std::vector<layout::Candidate>& candidates, std::size_t selected_index);
    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param);

  private:
    ~CandidateListUIElement();

    HRESULT show_native_window();
    void update_native_window() noexcept;
    void destroy_native_window() noexcept;
    static std::wstring to_wide(std::string_view utf8);
    void reset_page_index();
    void update_current_page_for_selection() noexcept;
    std::size_t current_page_start() const noexcept;
    std::size_t current_page_end() const noexcept;

    LONG reference_count_ = 1;
    wil::com_ptr<ITfContext> context_;
    std::vector<std::wstring> candidates_;
    std::vector<UINT> page_starts_;
    UINT selected_index_ = 0;
    UINT current_page_ = 0;
    DWORD updated_flags_ = TF_CLUIE_COUNT | TF_CLUIE_SELECTION | TF_CLUIE_STRING
                           | TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE;
    bool shown_ = false;
    HWND window_ = nullptr;
    FinalizeCallback finalize_callback_;
    AbortCallback abort_callback_;
};

}  // namespace suborno::platform::windows
