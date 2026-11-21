#include "suborno/platform/windows/tsf/candidate_list_ui_element.hpp"

#include <algorithm>
#include <iterator>
#include <limits>
#include <mutex>
#include <new>
#include <oleauto.h>
#include <string>
#include <string_view>

namespace suborno::platform::windows {
namespace {

constexpr wchar_t kCandidateWindowClass[] = L"SubornoCandidateListUIElement";
constexpr int kCandidateRowHeight = 26;
constexpr int kCandidateWindowWidth = 340;
constexpr std::size_t kDefaultCandidatesPerPage = 8;

HINSTANCE candidate_window_module_instance() noexcept {
    HMODULE module = nullptr;
    constexpr DWORD kModuleFlags =
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if (::GetModuleHandleExW(
            kModuleFlags, reinterpret_cast<LPCWSTR>(&CandidateListUIElement::window_proc), &module)
        == FALSE) {
        return nullptr;
    }
    return module;
}

ATOM register_candidate_window_class() {
    static std::once_flag registration;
    static ATOM registered_class = 0;
    std::call_once(registration, [] {
        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.lpfnWndProc = &CandidateListUIElement::window_proc;
        window_class.hInstance = candidate_window_module_instance();
        if (window_class.hInstance == nullptr) {
            return;
        }
        window_class.hCursor = ::LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        window_class.hbrBackground = ::GetSysColorBrush(COLOR_INFOBK);
        window_class.lpszClassName = kCandidateWindowClass;
        registered_class = ::RegisterClassExW(&window_class);
        if (registered_class == 0 && ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
            registered_class = 1;
        }
    });
    return registered_class;
}

}  // namespace

CandidateListUIElement::CandidateListUIElement(ITfContext* context,
                                               const std::vector<layout::Candidate>& candidates,
                                               std::size_t selected_index,
                                               FinalizeCallback finalize_callback,
                                               AbortCallback abort_callback)
    : context_(context),
      selected_index_(static_cast<UINT>(selected_index)),
      finalize_callback_(std::move(finalize_callback)),
      abort_callback_(std::move(abort_callback)) {
    candidates_.reserve(candidates.size());
    for (const layout::Candidate& candidate : candidates) {
        candidates_.push_back(to_wide(candidate.text));
    }
    if (candidates_.empty()) {
        selected_index_ = 0;
    } else if (selected_index_ >= candidates_.size()) {
        selected_index_ = static_cast<UINT>(candidates_.size() - 1);
    }
    reset_page_index();
    update_current_page_for_selection();
}

CandidateListUIElement::~CandidateListUIElement() {
    destroy_native_window();
}

std::wstring CandidateListUIElement::to_wide(std::string_view utf8) {
    if (utf8.empty() || utf8.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return {};
    }

    const int input_length = static_cast<int>(utf8.size());
    const int required =
        ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), input_length, nullptr, 0);
    if (required <= 0) {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(required), L'\0');
    if (::MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), input_length, result.data(), required)
        != required) {
        return {};
    }
    return result;
}

STDMETHODIMP CandidateListUIElement::QueryInterface(REFIID riid, void** object) {
    if (object == nullptr) {
        return E_POINTER;
    }
    *object = nullptr;

    if (riid == IID_IUnknown || riid == IID_ITfUIElement || riid == IID_ITfCandidateListUIElement) {
        *object = static_cast<ITfCandidateListUIElement*>(this);
    } else if (riid == IID_ITfCandidateListUIElementBehavior) {
        *object = static_cast<ITfCandidateListUIElementBehavior*>(this);
    } else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CandidateListUIElement::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&reference_count_));
}

STDMETHODIMP_(ULONG) CandidateListUIElement::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&reference_count_));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP CandidateListUIElement::GetDescription(BSTR* description) {
    if (description == nullptr) {
        return E_INVALIDARG;
    }
    *description = ::SysAllocString(L"Suborno candidate list");
    return *description == nullptr ? E_OUTOFMEMORY : S_OK;
}

STDMETHODIMP CandidateListUIElement::GetGUID(GUID* guid) {
    if (guid == nullptr) {
        return E_INVALIDARG;
    }
    *guid = kSubornoCandidateListUIElementGuid;
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::Show(BOOL show) {
    if (show != FALSE) {
        const HRESULT result = show_native_window();
        if (SUCCEEDED(result)) {
            shown_ = true;
        }
        return result;
    }

    shown_ = false;
    destroy_native_window();
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::IsShown(BOOL* show) {
    if (show == nullptr) {
        return E_INVALIDARG;
    }
    *show = shown_ ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::GetUpdatedFlags(DWORD* flags) {
    if (flags == nullptr) {
        return E_INVALIDARG;
    }
    *flags = updated_flags_;
    updated_flags_ = 0;
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::GetDocumentMgr(ITfDocumentMgr** document_manager) {
    if (document_manager == nullptr) {
        return E_INVALIDARG;
    }
    *document_manager = nullptr;
    if (context_ == nullptr) {
        return E_FAIL;
    }
    return context_->GetDocumentMgr(document_manager);
}

STDMETHODIMP CandidateListUIElement::GetCount(UINT* count) {
    if (count == nullptr) {
        return E_INVALIDARG;
    }
    *count = static_cast<UINT>(candidates_.size());
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::GetSelection(UINT* index) {
    if (index == nullptr) {
        return E_INVALIDARG;
    }
    *index = selected_index_;
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::GetString(UINT index, BSTR* text) {
    if (text == nullptr) {
        return E_INVALIDARG;
    }
    *text = nullptr;
    if (index >= candidates_.size()) {
        return E_INVALIDARG;
    }
    const std::wstring& candidate = candidates_[index];
    *text = ::SysAllocStringLen(candidate.data(), static_cast<UINT>(candidate.size()));
    return *text == nullptr ? E_OUTOFMEMORY : S_OK;
}

STDMETHODIMP CandidateListUIElement::GetPageIndex(UINT* index, UINT size, UINT* page_count) {
    if (page_count == nullptr) {
        return E_INVALIDARG;
    }
    *page_count = static_cast<UINT>(page_starts_.size());
    if (index == nullptr) {
        return S_OK;
    }
    if (size < page_starts_.size()) {
        return E_INVALIDARG;
    }
    std::copy(page_starts_.begin(), page_starts_.end(), index);
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::SetPageIndex(UINT* index, UINT page_count) {
    if (index == nullptr || page_count == 0 || candidates_.empty() || index[0] != 0) {
        return E_INVALIDARG;
    }

    std::vector<UINT> updated;
    updated.reserve(page_count);
    UINT previous = 0;
    for (UINT page = 0; page < page_count; ++page) {
        if (index[page] >= candidates_.size() || (page != 0 && index[page] <= previous)) {
            return E_INVALIDARG;
        }
        updated.push_back(index[page]);
        previous = index[page];
    }

    page_starts_ = std::move(updated);
    update_current_page_for_selection();
    updated_flags_ |= TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE;
    update_native_window();
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::GetCurrentPage(UINT* page) {
    if (page == nullptr) {
        return E_INVALIDARG;
    }
    *page = current_page_;
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::SetSelection(UINT index) {
    if (index >= candidates_.size()) {
        return E_INVALIDARG;
    }
    if (selected_index_ != index) {
        selected_index_ = index;
        updated_flags_ |= TF_CLUIE_SELECTION;
        const UINT previous_page = current_page_;
        update_current_page_for_selection();
        if (previous_page != current_page_) {
            updated_flags_ |= TF_CLUIE_CURRENTPAGE;
        }
        update_native_window();
    }
    return S_OK;
}

STDMETHODIMP CandidateListUIElement::Finalize() {
    AddRef();
    const HRESULT result = finalize_callback_ ? finalize_callback_(selected_index_) : E_NOTIMPL;
    if (SUCCEEDED(result)) {
        Show(FALSE);
    }
    Release();
    return result;
}

STDMETHODIMP CandidateListUIElement::Abort() {
    AddRef();
    const HRESULT result = abort_callback_ ? abort_callback_() : E_NOTIMPL;
    if (SUCCEEDED(result)) {
        Show(FALSE);
    }
    Release();
    return result;
}

void CandidateListUIElement::Update(const std::vector<layout::Candidate>& candidates,
                                    std::size_t selected_index) {
    candidates_.clear();
    candidates_.reserve(candidates.size());
    for (const layout::Candidate& candidate : candidates) {
        candidates_.push_back(to_wide(candidate.text));
    }

    const UINT new_selection =
        candidates_.empty() ? 0U
                            : static_cast<UINT>(std::min(selected_index, candidates_.size() - 1));
    updated_flags_ |= TF_CLUIE_COUNT | TF_CLUIE_STRING | TF_CLUIE_PAGEINDEX;
    const UINT previous_page = current_page_;
    reset_page_index();
    if (selected_index_ != new_selection) {
        selected_index_ = new_selection;
        updated_flags_ |= TF_CLUIE_SELECTION;
    }
    update_current_page_for_selection();
    if (previous_page != current_page_) {
        updated_flags_ |= TF_CLUIE_CURRENTPAGE;
    }
    update_native_window();
}

HRESULT CandidateListUIElement::show_native_window() {
    if (window_ != nullptr) {
        update_native_window();
        ::ShowWindow(window_, SW_SHOWNOACTIVATE);
        return S_OK;
    }
    if (register_candidate_window_class() == 0) {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    POINT anchor{};
    HWND owner = ::GetForegroundWindow();
    const DWORD thread_id =
        owner == nullptr ? ::GetCurrentThreadId() : ::GetWindowThreadProcessId(owner, nullptr);
    GUITHREADINFO gui_info{};
    gui_info.cbSize = sizeof(gui_info);
    if (::GetGUIThreadInfo(thread_id, &gui_info) != FALSE && gui_info.hwndCaret != nullptr) {
        anchor.x = gui_info.rcCaret.left;
        anchor.y = gui_info.rcCaret.bottom;
        if (::ClientToScreen(gui_info.hwndCaret, &anchor) == FALSE) {
            anchor = {};
        }
    }
    if (anchor.x == 0 && anchor.y == 0) {
        ::GetCursorPos(&anchor);
    }

    const std::size_t visible_count = current_page_end() - current_page_start();
    const int height =
        static_cast<int>(std::max<std::size_t>(visible_count, 1)) * kCandidateRowHeight + 8;
    const HMONITOR monitor = ::MonitorFromPoint(anchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (::GetMonitorInfoW(monitor, &monitor_info) != FALSE
        && anchor.y + height > monitor_info.rcWork.bottom) {
        anchor.y = std::max(monitor_info.rcWork.top, anchor.y - height - 24);
    }

    window_ = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                kCandidateWindowClass,
                                L"Suborno candidates",
                                WS_POPUP | WS_BORDER,
                                anchor.x,
                                anchor.y,
                                kCandidateWindowWidth,
                                height,
                                owner,
                                nullptr,
                                candidate_window_module_instance(),
                                this);
    if (window_ == nullptr) {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    ::ShowWindow(window_, SW_SHOWNOACTIVATE);
    ::SetWindowPos(window_,
                   HWND_TOPMOST,
                   anchor.x,
                   anchor.y,
                   kCandidateWindowWidth,
                   height,
                   SWP_NOACTIVATE | SWP_SHOWWINDOW);
    update_native_window();
    return S_OK;
}

void CandidateListUIElement::update_native_window() noexcept {
    if (window_ != nullptr) {
        ::InvalidateRect(window_, nullptr, TRUE);
        ::UpdateWindow(window_);
    }
}

void CandidateListUIElement::destroy_native_window() noexcept {
    if (window_ != nullptr && ::IsWindow(window_) != FALSE) {
        ::DestroyWindow(window_);
    }
}

void CandidateListUIElement::reset_page_index() {
    page_starts_.clear();
    for (std::size_t start = 0; start < candidates_.size(); start += kDefaultCandidatesPerPage) {
        page_starts_.push_back(static_cast<UINT>(start));
    }
    if (page_starts_.empty()) {
        current_page_ = 0;
    } else if (current_page_ >= page_starts_.size()) {
        current_page_ = static_cast<UINT>(page_starts_.size() - 1);
    }
}

void CandidateListUIElement::update_current_page_for_selection() noexcept {
    if (page_starts_.empty()) {
        current_page_ = 0;
        return;
    }
    const auto found = std::upper_bound(page_starts_.begin(), page_starts_.end(), selected_index_);
    current_page_ = static_cast<UINT>(
        std::distance(page_starts_.begin(), found == page_starts_.begin() ? found : found - 1));
}

std::size_t CandidateListUIElement::current_page_start() const noexcept {
    return page_starts_.empty() ? 0 : page_starts_[current_page_];
}

std::size_t CandidateListUIElement::current_page_end() const noexcept {
    if (page_starts_.empty()) {
        return 0;
    }
    return current_page_ + 1 < page_starts_.size() ? page_starts_[current_page_ + 1]
                                                   : candidates_.size();
}

LRESULT CALLBACK CandidateListUIElement::window_proc(HWND window,
                                                     UINT message,
                                                     WPARAM w_param,
                                                     LPARAM l_param) {
    auto* element =
        reinterpret_cast<CandidateListUIElement*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
        element = static_cast<CandidateListUIElement*>(create->lpCreateParams);
        if (element == nullptr) {
            return FALSE;
        }
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(element));
        element->window_ = window;
        element->AddRef();
    }
    if (element == nullptr) {
        return ::DefWindowProcW(window, message, w_param, l_param);
    }

    switch (message) {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_LBUTTONUP: {
        const int row = (static_cast<short>(HIWORD(l_param)) - 4) / kCandidateRowHeight;
        const std::size_t index =
            row < 0 ? element->candidates_.size()
                    : element->current_page_start() + static_cast<std::size_t>(row);
        if (index < element->current_page_end()) {
            element->SetSelection(static_cast<UINT>(index));
            element->Finalize();
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC device_context = ::BeginPaint(window, &paint);
        RECT client{};
        ::GetClientRect(window, &client);
        ::FillRect(device_context, &client, ::GetSysColorBrush(COLOR_INFOBK));
        ::SetBkMode(device_context, TRANSPARENT);

        const std::size_t page_start = element->current_page_start();
        const std::size_t page_end = element->current_page_end();
        for (std::size_t index = page_start; index < page_end; ++index) {
            const std::size_t row_index = index - page_start;
            RECT row{
                4,
                static_cast<LONG>(4 + row_index * kCandidateRowHeight),
                client.right - 4,
                static_cast<LONG>(4 + (row_index + 1) * kCandidateRowHeight),
            };
            if (index == element->selected_index_) {
                ::FillRect(device_context, &row, ::GetSysColorBrush(COLOR_HIGHLIGHT));
                ::SetTextColor(device_context, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
            } else {
                ::SetTextColor(device_context, ::GetSysColor(COLOR_INFOTEXT));
            }

            const std::wstring label =
                std::to_wstring(index + 1) + L". " + element->candidates_[index];
            ::DrawTextW(device_context,
                        label.c_str(),
                        static_cast<int>(label.size()),
                        &row,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        ::DrawEdge(device_context, &client, EDGE_RAISED, BF_RECT);
        ::EndPaint(window, &paint);
        return 0;
    }
    case WM_NCDESTROY: {
        element->window_ = nullptr;
        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        const LRESULT result = ::DefWindowProcW(window, message, w_param, l_param);
        element->Release();
        return result;
    }
    default:
        return ::DefWindowProcW(window, message, w_param, l_param);
    }
}

}  // namespace suborno::platform::windows
