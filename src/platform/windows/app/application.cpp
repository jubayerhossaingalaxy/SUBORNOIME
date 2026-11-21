#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "suborno/platform/windows/app/application.hpp"

#include <algorithm>
#include <array>
#include <commctrl.h>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <memory>
#include <msctf.h>
#include <new>
#include <shellapi.h>
#include <string>
#include <system_error>
#include <wil/com.h>

#include "suborno/core/engine.hpp"
#include "suborno/platform/windows/app/resource.h"
#include "suborno/platform/windows/app_paths.hpp"
#include "suborno/platform/windows/tsf/guids.hpp"
#include "suborno_version.hpp"

namespace suborno::platform::windows::app {
namespace {

constexpr wchar_t kWindowClass[] = L"SubornoDesktopWindow";
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr LANGID kBengaliBangladesh = MAKELANGID(0x45, 0x02);
constexpr wchar_t kStartupKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kStartupValue[] = L"Suborno";

enum ControlId : UINT {
    kStatusLabel = 1001,
    kLayoutCombo = 1002,
    kStartupCheck = 1003,
    kOpenInputSettings = 1004,
    kHideButton = 1005,
    kAppsSettingsButton = 1006,
    kMapNumpadCheck = 1007,
    kIgnoreCapsLockCheck = 1008,
    kSelectCandidateKeysCheck = 1009,
    kShowCandidateWindowCheck = 1010,
    kLearningEnabledCheck = 1011,
};

enum MenuId : UINT {
    kMenuOpen = 2001,
    kMenuToggleInput = 2002,
    kMenuOpenSettings = 2003,
    kMenuApps = 2004,
    kMenuExit = 2005,
    kMenuLayoutFirst = 2100,
};

struct LayoutOption {
    const char* id;
    const wchar_t* name;
};

constexpr std::array<LayoutOption, 6> kLayouts{{
    {"suborno-phonetic", L"Suborno Phonetic"},
    {"borno-phonetic", L"Borno Phonetic"},
    {"avro-phonetic", L"Avro Phonetic"},
    {"khipro", L"Khipro Classic"},
    {"probhat", L"Probhat"},
    {"national", L"National (draft)"},
}};

HRESULT get_profiles(wil::com_ptr<ITfInputProcessorProfiles>& profiles) {
    return ::CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                              nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(profiles.put()));
}

HRESULT profile_enabled(bool& enabled) {
    enabled = false;
    wil::com_ptr<ITfInputProcessorProfiles> profiles;
    HRESULT result = get_profiles(profiles);
    if (FAILED(result)) {
        return result;
    }
    BOOL is_enabled = FALSE;
    result = profiles->IsEnabledLanguageProfile(
        suborno::platform::windows::CLSID_SubornoTextService,
        kBengaliBangladesh,
        suborno::platform::windows::GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI,
        &is_enabled);
    if (SUCCEEDED(result)) {
        enabled = is_enabled != FALSE;
    }
    return result;
}

HRESULT set_profile_enabled(bool enabled) {
    wil::com_ptr<ITfInputProcessorProfiles> profiles;
    const HRESULT result = get_profiles(profiles);
    if (FAILED(result)) {
        return result;
    }
    return profiles->EnableLanguageProfile(
        suborno::platform::windows::CLSID_SubornoTextService,
        kBengaliBangladesh,
        suborno::platform::windows::GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI,
        enabled ? TRUE : FALSE);
}

HRESULT refresh_active_profile() {
    wil::com_ptr<ITfInputProcessorProfiles> profiles;
    HRESULT result = get_profiles(profiles);
    if (FAILED(result)) {
        return result;
    }

    LANGID language = 0;
    GUID profile = GUID_NULL;
    result = profiles->GetActiveLanguageProfile(
        suborno::platform::windows::CLSID_SubornoTextService, &language, &profile);
    if (result == S_FALSE) {
        return S_OK;
    }
    if (FAILED(result)) {
        return result;
    }
    return profiles->ActivateLanguageProfile(
        suborno::platform::windows::CLSID_SubornoTextService, language, profile);
}

HRESULT executable_path(std::wstring& path) {
    // Use heap allocation to avoid stack overflow (C6262)
    std::vector<wchar_t> buffer(32768);
    const DWORD length =
        ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    if (length >= static_cast<DWORD>(buffer.size())) {
        // Path exceeds buffer size; try a larger allocation
        std::vector<wchar_t> larger_buffer(length + 1);
        const DWORD larger_length = ::GetModuleFileNameW(
            nullptr, larger_buffer.data(), static_cast<DWORD>(larger_buffer.size()));
        if (larger_length == 0) {
            return HRESULT_FROM_WIN32(::GetLastError());
        }
        path.assign(larger_buffer.data(), larger_length);
        return S_OK;
    }
    path.assign(buffer.data(), length);
    return S_OK;
}

HRESULT read_startup_preference(bool& enabled) {
    enabled = false;
    HKEY key = nullptr;
    const LONG open_result =
        ::RegOpenKeyExW(HKEY_CURRENT_USER, kStartupKey, 0, KEY_QUERY_VALUE, &key);
    if (open_result == ERROR_FILE_NOT_FOUND || open_result == ERROR_PATH_NOT_FOUND) {
        return S_OK;
    }
    if (open_result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(open_result);
    }

    DWORD type = 0;
    DWORD size = 0;
    const LONG query_result =
        ::RegQueryValueExW(key, kStartupValue, nullptr, &type, nullptr, &size);
    if (query_result == ERROR_FILE_NOT_FOUND) {
        ::RegCloseKey(key);
        return S_OK;
    }
    if (query_result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        ::RegCloseKey(key);
        return query_result == ERROR_SUCCESS ? HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
                                             : HRESULT_FROM_WIN32(query_result);
    }
    enabled = size > sizeof(wchar_t);
    ::RegCloseKey(key);
    return S_OK;
}

HRESULT write_startup_preference(bool enabled) {
    HKEY key = nullptr;
    const LONG create_result = ::RegCreateKeyExW(
        HKEY_CURRENT_USER, kStartupKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (create_result != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(create_result);
    }

    LONG result = ERROR_SUCCESS;
    if (enabled) {
        std::wstring path;
        const HRESULT path_result = executable_path(path);
        if (FAILED(path_result)) {
            ::RegCloseKey(key);
            return path_result;
        }
        const std::wstring command = L"\"" + path + L"\" --startup";
        result = ::RegSetValueExW(key,
                                  kStartupValue,
                                  0,
                                  REG_SZ,
                                  reinterpret_cast<const BYTE*>(command.c_str()),
                                  static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        result = ::RegDeleteValueW(key, kStartupValue);
        if (result == ERROR_FILE_NOT_FOUND) {
            result = ERROR_SUCCESS;
        }
    }
    ::RegCloseKey(key);
    return result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(result);
}

HRESULT open_uri(const wchar_t* uri) {
    const HINSTANCE result =
        ::ShellExecuteW(nullptr, L"open", uri, nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32 ? S_OK : E_FAIL;
}

void show_error(HWND owner, const wchar_t* message, HRESULT result) {
    wchar_t text[512]{};
    swprintf_s(
        text, _countof(text), L"%s\n\nError: 0x%08lX", message, static_cast<unsigned long>(result));
    ::MessageBoxW(owner, text, L"Suborno", MB_OK | MB_ICONERROR);
}

void report_error(HWND owner, const wchar_t* message, HRESULT result, bool show_dialog) {
    if (show_dialog) {
        show_error(owner, message, result);
        return;
    }

    wchar_t text[512]{};
    swprintf_s(text,
               _countof(text),
               L"%s (HRESULT: 0x%08lX)\n",
               message,
               static_cast<unsigned long>(result));
    ::OutputDebugStringW(text);
}

int smoke_test_exit_code(HRESULT result) noexcept {
    const DWORD code = HRESULT_CODE(result);
    return code == ERROR_SUCCESS ? 1 : static_cast<int>(code);
}

class Application final {
  public:
    Application(HINSTANCE instance, bool startup, bool smoke_test)
        : instance_(instance),
          startup_(startup),
          smoke_test_(smoke_test),
          taskbar_created_message_(::RegisterWindowMessageW(L"TaskbarCreated")) {}

    int run(int show_command) {
        HRESULT result = initialize();
        if (FAILED(result)) {
            report_error(nullptr, L"Suborno could not load its settings.", result, !smoke_test_);
            return smoke_test_ ? smoke_test_exit_code(result) : 1;
        }

        WNDCLASSEXW window_class{};
        window_class.cbSize = sizeof(window_class);
        window_class.hInstance = instance_;
        window_class.lpfnWndProc = &Application::window_proc;
        window_class.lpszClassName = kWindowClass;
        window_class.hIcon = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_SUBORNO));
        window_class.hIconSm = window_class.hIcon;
        window_class.hCursor = ::LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        if (window_class.hIcon == nullptr) {
            report_error(
                nullptr, L"Suborno's application icon could not be loaded.", E_FAIL, !smoke_test_);
            return smoke_test_ ? smoke_test_exit_code(E_FAIL) : 1;
        }
        if (::RegisterClassExW(&window_class) == 0) {
            const HRESULT error = HRESULT_FROM_WIN32(::GetLastError());
            report_error(nullptr,
                         L"Suborno's application window could not be registered.",
                         error,
                         !smoke_test_);
            return smoke_test_ ? smoke_test_exit_code(error) : 1;
        }

        window_ = ::CreateWindowExW(0,
                                    kWindowClass,
                                    L"Suborno",
                                    WS_OVERLAPPEDWINDOW,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    600,
                                    600,
                                    nullptr,
                                    nullptr,
                                    instance_,
                                    this);
        if (window_ == nullptr) {
            const HRESULT error = HRESULT_FROM_WIN32(::GetLastError());
            report_error(nullptr,
                         L"Suborno's application window could not be created.",
                         error,
                         !smoke_test_);
            ::UnregisterClassW(kWindowClass, instance_);
            return smoke_test_ ? smoke_test_exit_code(error) : 1;
        }
        if (smoke_test_) {
            ::DestroyWindow(window_);
            ::UnregisterClassW(kWindowClass, instance_);
            return 0;
        }

        if (!startup_) {
            ::ShowWindow(window_, show_command);
            ::UpdateWindow(window_);
        }

        MSG message{};
        int message_result = 0;
        while ((message_result = ::GetMessageW(&message, nullptr, 0, 0)) > 0) {
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }
        ::UnregisterClassW(kWindowClass, instance_);
        if (message_result == -1) {
            return 1;
        }
        return static_cast<int>(message.wParam);
    }

  private:
    HRESULT initialize() {
        try {
            std::filesystem::path user_data;
            std::filesystem::path legacy_user_data;
            HRESULT result = get_user_data_directory(user_data);
            if (FAILED(result)) {
                return result;
            }
            result = get_legacy_user_data_directory(legacy_user_data);
            if (FAILED(result)) {
                return result;
            }
            result = prepare_user_data_directory(user_data, legacy_user_data);
            if (FAILED(result)) {
                return result;
            }
            suborno::core::EngineConfig config;
            std::filesystem::path settings_path;
            result = get_user_settings_path(settings_path);
            if (FAILED(result)) {
                return result;
            }
            config.settings_path = settings_path;
            std::filesystem::path dictionary_path;
            result = get_user_dictionary_path(dictionary_path);
            if (FAILED(result)) {
                return result;
            }
            config.dictionary_path = dictionary_path;
            std::filesystem::path shared_dictionaries;
            result = get_shared_dictionaries_directory(shared_dictionaries);
            if (FAILED(result)) {
                return result;
            }
            config.dictionary_word_lists = {
                shared_dictionaries / L"bn-wordlist-unlicense-v1-part-01.txt",
                shared_dictionaries / L"bn-wordlist-unlicense-v1-part-02.txt",
                shared_dictionaries / L"bn-wordlist-unlicense-v1-part-03.txt",
                shared_dictionaries / L"bn-wordlist-unlicense-v1-part-04.txt"};
            config.dictionary_corpus_version = "sanjidh090-bn-wordlist-d3095ea-v1";
            engine_ = std::make_unique<suborno::core::Engine>(config);
            const auto loaded = engine_->load_settings();
            if (loaded.status != suborno::settings::SettingsStatus::ok) {
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
            result = read_startup_preference(startup_enabled_);
            if (FAILED(result)) {
                return result;
            }

            auto settings = engine_->settings();
            if (settings.general.start_with_windows != startup_enabled_) {
                settings.general.start_with_windows = startup_enabled_;
                if (engine_->save_settings(settings) != suborno::settings::SettingsStatus::ok) {
                    return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
                }
            }
            return S_OK;
        } catch (const std::bad_alloc&) {
            return E_OUTOFMEMORY;
        } catch (const std::exception&) {
            return E_FAIL;
        }
    }

    static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
        Application* app =
            reinterpret_cast<Application*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
            app = static_cast<Application*>(create->lpCreateParams);
            app->window_ = window;
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        if (app == nullptr) {
            return ::DefWindowProcW(window, message, w_param, l_param);
        }
        return app->handle_message(message, w_param, l_param);
    }

    LRESULT handle_message(UINT message, WPARAM w_param, LPARAM l_param) {
        if (message == taskbar_created_message_) {
            const HRESULT result = add_tray_icon();
            if (FAILED(result)) {
                show_error(
                    window_, L"Suborno could not restore its notification area icon.", result);
            }
            refresh_status();
            return 0;
        }
        switch (message) {
        case WM_CREATE:
            if (!create_controls()) {
                return -1;
            }
            if (!smoke_test_) {
                const HRESULT result = add_tray_icon();
                if (FAILED(result)) {
                    ::SetWindowTextW(status_label_,
                                     L"Notification area integration is unavailable.");
                    ::EnableWindow(hide_button_, FALSE);
                    show_error(
                        window_, L"Suborno could not add its notification area icon.", result);
                }
            }
            refresh_status();
            return 0;
        case WM_COMMAND:
            handle_command(LOWORD(w_param), HIWORD(w_param));
            return 0;
        case WM_ACTIVATE:
            if (LOWORD(w_param) != WA_INACTIVE) {
                refresh_status();
            }
            return ::DefWindowProcW(window_, message, w_param, l_param);
        case WM_CLOSE:
            if (!smoke_test_ && tray_added_) {
                ::ShowWindow(window_, SW_HIDE);
                return 0;
            }
            ::DestroyWindow(window_);
            return 0;
        case kTrayMessage:
            handle_tray_message(l_param);
            return 0;
        case WM_DESTROY:
            remove_tray_icon();
            ::PostQuitMessage(0);
            return 0;
        default:
            return ::DefWindowProcW(window_, message, w_param, l_param);
        }
    }

    HWND add_control(DWORD extended_style,
                     const wchar_t* class_name,
                     const wchar_t* text,
                     DWORD style,
                     int x,
                     int y,
                     int width,
                     int height,
                     UINT id) {
        HWND control = ::CreateWindowExW(extended_style,
                                         class_name,
                                         text,
                                         WS_CHILD | WS_VISIBLE | style,
                                         x,
                                         y,
                                         width,
                                         height,
                                         window_,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                         instance_,
                                         nullptr);
        if (control != nullptr) {
            ::SendMessageW(control,
                           WM_SETFONT,
                           reinterpret_cast<WPARAM>(::GetStockObject(DEFAULT_GUI_FONT)),
                           TRUE);
        }
        return control;
    }

    bool create_controls() {
        add_control(0, L"STATIC", L"Input method status", SS_LEFT, 24, 20, 520, 24, 0);
        status_label_ = add_control(0,
                                    L"STATIC",
                                    L"Checking Windows input method status...",
                                    SS_LEFT,
                                    24,
                                    48,
                                    520,
                                    28,
                                    kStatusLabel);

        add_control(0, L"STATIC", L"Keyboard layout", SS_LEFT, 24, 92, 150, 24, 0);
        layout_combo_ = add_control(
            0, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, 180, 88, 340, 180, kLayoutCombo);
        if (status_label_ == nullptr || layout_combo_ == nullptr) {
            return false;
        }
        for (std::size_t index = 0; index < kLayouts.size(); ++index) {
            const LRESULT item = ::SendMessageW(
                layout_combo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kLayouts[index].name));
            if (item == CB_ERR
                || ::SendMessageW(layout_combo_,
                                  CB_SETITEMDATA,
                                  static_cast<WPARAM>(item),
                                  static_cast<LPARAM>(index))
                       == CB_ERR) {
                return false;
            }
        }

        startup_check_ = add_control(0,
                                     L"BUTTON",
                                     L"Launch Suborno when I sign in",
                                     BS_AUTOCHECKBOX,
                                     24,
                                     132,
                                     330,
                                     28,
                                     kStartupCheck);
        ::SendMessageW(
            startup_check_, BM_SETCHECK, startup_enabled_ ? BST_CHECKED : BST_UNCHECKED, 0);

        add_control(0, L"STATIC", L"Input preferences", SS_LEFT, 24, 170, 520, 24, 0);
        show_candidate_window_check_ = add_control(0,
                                                   L"BUTTON",
                                                   L"Show candidate suggestion window",
                                                   BS_AUTOCHECKBOX,
                                                   24,
                                                   196,
                                                   500,
                                                   26,
                                                   kShowCandidateWindowCheck);
        map_numpad_check_ = add_control(0,
                                        L"BUTTON",
                                        L"Map numeric keypad to Bangla digits",
                                        BS_AUTOCHECKBOX,
                                        24,
                                        224,
                                        500,
                                        26,
                                        kMapNumpadCheck);
        ignore_caps_lock_check_ = add_control(0,
                                              L"BUTTON",
                                              L"Ignore Caps Lock while typing Bangla",
                                              BS_AUTOCHECKBOX,
                                              24,
                                              252,
                                              500,
                                              26,
                                              kIgnoreCapsLockCheck);
        select_candidate_keys_check_ = add_control(0,
                                                   L"BUTTON",
                                                   L"Select candidates with number keys",
                                                   BS_AUTOCHECKBOX,
                                                   24,
                                                   280,
                                                   500,
                                                   26,
                                                   kSelectCandidateKeysCheck);
        learning_check_ = add_control(0,
                                      L"BUTTON",
                                      L"Learn committed words on this device (opt-in)",
                                      BS_AUTOCHECKBOX,
                                      24,
                                      308,
                                      500,
                                      26,
                                      kLearningEnabledCheck);
        if (show_candidate_window_check_ == nullptr || map_numpad_check_ == nullptr
            || ignore_caps_lock_check_ == nullptr || select_candidate_keys_check_ == nullptr
            || learning_check_ == nullptr) {
            return false;
        }
        refresh_input_settings_controls();

        add_control(0, L"STATIC", L"Typing help", SS_LEFT, 24, 342, 520, 24, 0);
        help_label_ = add_control(0, L"STATIC", L"", SS_LEFT, 24, 368, 520, 38, 0);

        add_control(0,
                    L"BUTTON",
                    L"Windows input settings",
                    BS_PUSHBUTTON,
                    24,
                    414,
                    190,
                    32,
                    kOpenInputSettings);
        hide_button_ = add_control(0,
                                   L"BUTTON",
                                   L"Hide to notification area",
                                   BS_PUSHBUTTON,
                                   228,
                                   414,
                                   220,
                                   32,
                                   kHideButton);

        add_control(0, L"STATIC", L"About Suborno", SS_LEFT, 24, 460, 520, 22, 0);
        add_control(0,
                    L"STATIC",
                    L"Suborno Bengali Input Method  " SUBORNO_VERSION_WIDE,
                    SS_LEFT,
                    24,
                    486,
                    520,
                    24,
                    0);
        apps_settings_button_ = add_control(0,
                                            L"BUTTON",
                                            L"Uninstall Suborno",
                                            BS_PUSHBUTTON,
                                            24,
                                            518,
                                            220,
                                            30,
                                            kAppsSettingsButton);

        for (std::size_t index = 0; index < kLayouts.size(); ++index) {
            if (engine_->settings().active_layout_id == kLayouts[index].id) {
                ::SendMessageW(layout_combo_, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
                break;
            }
        }
        return status_label_ != nullptr && layout_combo_ != nullptr && startup_check_ != nullptr
               && hide_button_ != nullptr && apps_settings_button_ != nullptr
               && show_candidate_window_check_ != nullptr && map_numpad_check_ != nullptr
               && ignore_caps_lock_check_ != nullptr && select_candidate_keys_check_ != nullptr
               && learning_check_ != nullptr && help_label_ != nullptr;
    }

    void refresh_input_settings_controls() {
        const auto& settings = engine_->settings();
        const auto& input = settings.input;
        ::SendMessageW(
            show_candidate_window_check_,
            BM_SETCHECK,
            settings.interface_settings.show_candidate_window ? BST_CHECKED : BST_UNCHECKED,
            0);
        ::SendMessageW(map_numpad_check_,
                       BM_SETCHECK,
                       input.map_numpad_to_bangla_numbers ? BST_CHECKED : BST_UNCHECKED,
                       0);
        ::SendMessageW(ignore_caps_lock_check_,
                       BM_SETCHECK,
                       input.ignore_caps_lock_state_in_bangla ? BST_CHECKED : BST_UNCHECKED,
                       0);
        ::SendMessageW(select_candidate_keys_check_,
                       BM_SETCHECK,
                       input.select_candidate_using_number_keys ? BST_CHECKED : BST_UNCHECKED,
                       0);
        ::SendMessageW(learning_check_,
                       BM_SETCHECK,
                       settings.learning_enabled ? BST_CHECKED : BST_UNCHECKED,
                       0);
    }

    void save_input_settings() {
        auto settings = engine_->settings();
        settings.interface_settings.show_candidate_window =
            ::SendMessageW(show_candidate_window_check_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        settings.input.map_numpad_to_bangla_numbers =
            ::SendMessageW(map_numpad_check_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        settings.input.ignore_caps_lock_state_in_bangla =
            ::SendMessageW(ignore_caps_lock_check_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        settings.input.select_candidate_using_number_keys =
            ::SendMessageW(select_candidate_keys_check_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        settings.learning_enabled =
            ::SendMessageW(learning_check_, BM_GETCHECK, 0, 0) == BST_CHECKED;

        const auto result = engine_->save_settings(settings);
        if (result != suborno::settings::SettingsStatus::ok) {
            refresh_input_settings_controls();
            show_error(window_, L"Could not save the input preferences.", E_FAIL);
        }
    }

    void refresh_status() {
        refresh_input_settings_controls();
        const std::string_view active_layout = engine_->settings().active_layout_id;
        const bool phonetic = active_layout == "suborno-phonetic"
                              || active_layout == "borno-phonetic"
                              || active_layout == "avro-phonetic";
        ::SetWindowTextW(
            help_label_,
            phonetic
                ? L"Type with English letters. Space commits; 1-9 selects a candidate; Esc cancels."
                : L"Use the selected layout. Space commits; 1-9 selects a candidate; Esc cancels.");

        bool enabled = false;
        const HRESULT result = profile_enabled(enabled);
        if (FAILED(result)) {
            wchar_t status[160]{};
            swprintf_s(status,
                       _countof(status),
                       L"Windows input status is unavailable (0x%08lX).",
                       static_cast<unsigned long>(result));
            ::SetWindowTextW(status_label_, status);
            return;
        }

        const auto layout =
            std::find_if(kLayouts.begin(), kLayouts.end(), [this](const auto& item) {
                return engine_->settings().active_layout_id == item.id;
            });
        const wchar_t* layout_name = layout == kLayouts.end() ? L"Unknown" : layout->name;
        wchar_t status[256]{};
        swprintf_s(status,
                   _countof(status),
                   L"Suborno is %s in Windows input methods. Current layout: %s.",
                   enabled ? L"enabled" : L"disabled",
                   layout_name);
        ::SetWindowTextW(status_label_, status);
        update_tray_tooltip(enabled, layout_name);
    }

    void handle_command(UINT id, UINT notification) {
        if (id == kLayoutCombo && notification == CBN_SELCHANGE) {
            apply_selected_layout();
            return;
        }
        if ((id == kShowCandidateWindowCheck || id == kMapNumpadCheck || id == kIgnoreCapsLockCheck
             || id == kSelectCandidateKeysCheck || id == kLearningEnabledCheck)
            && notification == BN_CLICKED) {
            save_input_settings();
            return;
        }
        if (id == kStartupCheck && notification == BN_CLICKED) {
            const bool enabled = ::SendMessageW(startup_check_, BM_GETCHECK, 0, 0) == BST_CHECKED;
            const HRESULT registry_result = write_startup_preference(enabled);
            HRESULT result = registry_result;
            if (SUCCEEDED(result)) {
                auto settings = engine_->settings();
                settings.general.start_with_windows = enabled;
                if (engine_->save_settings(settings) != suborno::settings::SettingsStatus::ok) {
                    result = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
                    const HRESULT rollback_result = write_startup_preference(startup_enabled_);
                    if (FAILED(rollback_result)) {
                        result = rollback_result;
                    }
                }
            }
            if (FAILED(result)) {
                ::SendMessageW(
                    startup_check_, BM_SETCHECK, startup_enabled_ ? BST_CHECKED : BST_UNCHECKED, 0);
                show_error(window_, L"Could not update the sign-in launch preference.", result);
            } else {
                startup_enabled_ = enabled;
            }
            return;
        }
        switch (id) {
        case kOpenInputSettings:
            if (FAILED(open_uri(L"ms-settings:regionlanguage"))) {
                show_error(window_, L"Could not open Windows Language & region settings.", E_FAIL);
            }
            break;
        case kHideButton:
            ::ShowWindow(window_, SW_HIDE);
            break;
        case kAppsSettingsButton:
            open_uninstall_options();
            break;
        case kMenuOpen:
        case kMenuOpenSettings:
            show_window();
            break;
        case kMenuToggleInput:
            toggle_profile();
            break;
        case kMenuApps:
            open_uninstall_options();
            break;
        case kMenuExit:
            ::DestroyWindow(window_);
            break;
        default:
            if (id >= kMenuLayoutFirst
                && id < kMenuLayoutFirst + static_cast<int>(kLayouts.size())) {
                save_layout(kLayouts[static_cast<std::size_t>(id - kMenuLayoutFirst)].id);
            }
            break;
        }
    }

    void apply_selected_layout() {
        const LRESULT selection = ::SendMessageW(layout_combo_, CB_GETCURSEL, 0, 0);
        if (selection == CB_ERR || selection < 0
            || selection >= static_cast<LRESULT>(kLayouts.size())) {
            return;
        }
        const auto index = static_cast<std::size_t>(selection);
        save_layout(kLayouts[index].id);
    }

    void save_layout(const char* layout_id) {
        auto settings = engine_->settings();
        settings.active_layout_id = layout_id;
        const auto result = engine_->save_settings(settings);
        if (result != suborno::settings::SettingsStatus::ok) {
            show_error(window_, L"Could not save the selected keyboard layout.", E_FAIL);
            return;
        }
        const HRESULT refresh_result = refresh_active_profile();
        if (FAILED(refresh_result)) {
            show_error(window_,
                       L"Layout saved, but Windows could not refresh the active profile.",
                       refresh_result);
        }
        refresh_status();
    }

    void open_uninstall_options() {
        const HINSTANCE result =
            ::ShellExecuteW(nullptr, L"open", L"appwiz.cpl", nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(result) <= 32) {
            show_error(window_, L"Could not open Windows uninstall options.", E_FAIL);
        }
    }

    void toggle_profile() {
        bool enabled = false;
        HRESULT result = profile_enabled(enabled);
        if (SUCCEEDED(result)) {
            result = set_profile_enabled(!enabled);
        }
        if (FAILED(result)) {
            show_error(window_, L"Could not update Suborno's Windows input-method status.", result);
            return;
        }
        refresh_status();
    }

    HRESULT add_tray_icon() {
        tray_data_.cbSize = sizeof(tray_data_);
        tray_data_.hWnd = window_;
        tray_data_.uID = kTrayIconId;
        tray_data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        tray_data_.uCallbackMessage = kTrayMessage;
        tray_data_.hIcon = ::LoadIconW(instance_, MAKEINTRESOURCEW(IDI_SUBORNO));
        if (tray_data_.hIcon == nullptr) {
            return E_FAIL;
        }
        ::wcscpy_s(tray_data_.szTip, _countof(tray_data_.szTip), L"Suborno");
        tray_added_ = ::Shell_NotifyIconW(NIM_ADD, &tray_data_) != FALSE;
        if (!tray_added_) {
            const DWORD error = ::GetLastError();
            return error == ERROR_SUCCESS ? E_FAIL : HRESULT_FROM_WIN32(error);
        }
        tray_data_.uVersion = NOTIFYICON_VERSION_4;
        if (!::Shell_NotifyIconW(NIM_SETVERSION, &tray_data_)) {
            const DWORD error = ::GetLastError();
            const HRESULT result = error == ERROR_SUCCESS ? E_FAIL : HRESULT_FROM_WIN32(error);
            remove_tray_icon();
            return result;
        }
        return S_OK;
    }

    void remove_tray_icon() {
        if (tray_added_) {
            ::Shell_NotifyIconW(NIM_DELETE, &tray_data_);
            tray_added_ = false;
        }
    }

    void update_tray_tooltip(bool enabled, const wchar_t* layout) {
        if (!tray_added_) {
            return;
        }
        tray_data_.uFlags = NIF_TIP;
        swprintf_s(tray_data_.szTip,
                   _countof(tray_data_.szTip),
                   L"Suborno - %s - %s",
                   enabled ? L"enabled" : L"disabled",
                   layout);
        ::Shell_NotifyIconW(NIM_MODIFY, &tray_data_);
    }

    void handle_tray_message(LPARAM event) {
        const UINT event_code = LOWORD(event);
        if (event_code == WM_LBUTTONUP || event_code == WM_LBUTTONDBLCLK || event_code == NIN_SELECT
            || event_code == NIN_KEYSELECT) {
            show_window();
        } else if (event_code == WM_RBUTTONUP || event_code == WM_CONTEXTMENU) {
            show_tray_menu();
        }
    }

    void show_tray_menu() {
        HMENU menu = ::CreatePopupMenu();
        HMENU layouts = ::CreatePopupMenu();
        if (menu == nullptr || layouts == nullptr) {
            if (menu != nullptr) {
                ::DestroyMenu(menu);
            }
            if (layouts != nullptr) {
                ::DestroyMenu(layouts);
            }
            return;
        }
        bool enabled = false;
        const HRESULT result = profile_enabled(enabled);
        const wchar_t* toggle_label = SUCCEEDED(result) && enabled ? L"Disable Suborno in Windows"
                                                                   : L"Enable Suborno in Windows";
        ::AppendMenuW(menu, MF_STRING, kMenuOpen, L"Open Suborno");
        ::AppendMenuW(menu, MF_STRING, kMenuToggleInput, toggle_label);
        ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        for (std::size_t index = 0; index < kLayouts.size(); ++index) {
            const bool selected = engine_->settings().active_layout_id == kLayouts[index].id;
            ::AppendMenuW(layouts,
                          MF_STRING | (selected ? MF_CHECKED : MF_UNCHECKED),
                          kMenuLayoutFirst + static_cast<UINT>(index),
                          kLayouts[index].name);
        }
        ::AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(layouts), L"Keyboard layout");
        ::AppendMenuW(menu, MF_STRING, kMenuOpenSettings, L"Settings");
        ::AppendMenuW(menu, MF_STRING, kMenuApps, L"Uninstall Suborno");
        ::AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        ::AppendMenuW(menu, MF_STRING, kMenuExit, L"Exit");

        POINT cursor{};
        ::GetCursorPos(&cursor);
        ::SetForegroundWindow(window_);
        ::TrackPopupMenu(menu,
                         TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                         cursor.x,
                         cursor.y,
                         0,
                         window_,
                         nullptr);
        ::PostMessageW(window_, WM_NULL, 0, 0);
        ::DestroyMenu(menu);
    }

    void show_window() {
        ::ShowWindow(window_, SW_RESTORE);
        ::SetForegroundWindow(window_);
        ::SetFocus(layout_combo_);
    }

    HINSTANCE instance_ = nullptr;
    HWND window_ = nullptr;
    HWND status_label_ = nullptr;
    HWND help_label_ = nullptr;
    HWND layout_combo_ = nullptr;
    HWND startup_check_ = nullptr;
    HWND show_candidate_window_check_ = nullptr;
    HWND map_numpad_check_ = nullptr;
    HWND ignore_caps_lock_check_ = nullptr;
    HWND select_candidate_keys_check_ = nullptr;
    HWND learning_check_ = nullptr;
    HWND hide_button_ = nullptr;
    HWND apps_settings_button_ = nullptr;
    bool startup_ = false;
    bool smoke_test_ = false;
    bool startup_enabled_ = false;
    bool tray_added_ = false;
    UINT taskbar_created_message_ = 0;
    NOTIFYICONDATAW tray_data_{};
    std::unique_ptr<suborno::core::Engine> engine_;
};

}  // namespace

int run(HINSTANCE instance, PWSTR command_line, int show_command) {
    const bool startup =
        command_line != nullptr && std::wcsstr(command_line, L"--startup") != nullptr;
    const bool smoke_test =
        command_line != nullptr && std::wcsstr(command_line, L"--smoke-test") != nullptr;

    HANDLE instance_mutex = ::CreateMutexW(nullptr, TRUE, L"Local\\Suborno.Desktop.Application");
    if (instance_mutex == nullptr) {
        const HRESULT error = HRESULT_FROM_WIN32(::GetLastError());
        report_error(
            nullptr, L"Suborno could not establish its single-instance lock.", error, !smoke_test);
        return smoke_test ? smoke_test_exit_code(error) : 1;
    }
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        for (int attempt = 0; attempt < 20; ++attempt) {
            HWND existing = ::FindWindowW(kWindowClass, nullptr);
            if (existing != nullptr) {
                ::ShowWindow(existing, SW_RESTORE);
                ::SetForegroundWindow(existing);
                break;
            }
            ::Sleep(50);
        }
        ::CloseHandle(instance_mutex);
        return 0;
    }

    const HRESULT com_result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        report_error(
            nullptr, L"Suborno could not initialize Windows services.", com_result, !smoke_test);
        ::ReleaseMutex(instance_mutex);
        ::CloseHandle(instance_mutex);
        return smoke_test ? smoke_test_exit_code(com_result) : 1;
    }
    int exit_code = 1;
    try {
        Application application(instance, startup, smoke_test);
        exit_code = application.run(show_command);
    } catch (const std::bad_alloc&) {
        report_error(nullptr, L"Suborno ran out of memory.", E_OUTOFMEMORY, !smoke_test);
        exit_code = smoke_test ? smoke_test_exit_code(E_OUTOFMEMORY) : 1;
    } catch (const std::filesystem::filesystem_error&) {
        report_error(nullptr, L"Suborno could not access its user data.", E_FAIL, !smoke_test);
        exit_code = smoke_test ? smoke_test_exit_code(E_FAIL) : 1;
    } catch (const std::exception&) {
        report_error(
            nullptr, L"Suborno could not initialize its input engine.", E_FAIL, !smoke_test);
        exit_code = smoke_test ? smoke_test_exit_code(E_FAIL) : 1;
    }

    ::CoUninitialize();
    ::ReleaseMutex(instance_mutex);
    ::CloseHandle(instance_mutex);
    return exit_code;
}

}  // namespace suborno::platform::windows::app
