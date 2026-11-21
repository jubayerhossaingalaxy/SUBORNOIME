#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace suborno::settings {

enum class StartMode : std::uint8_t {
    last_state,
    topbar,
    system_tray,
    main_window,
};

enum class SwitchingMode : std::uint8_t {
    per_application_window,
    global,
};

enum class CandidateOrientation : std::uint8_t {
    vertical,
    horizontal,
};

enum class TextEncoding : std::uint8_t {
    unicode,
    ansi,
};

struct GeneralSettings {
    bool remember_last_input_layout = true;
    bool start_with_windows = false;
    StartMode start_mode = StartMode::last_state;
    SwitchingMode switching_mode = SwitchingMode::per_application_window;
};

struct InterfaceSettings {
    std::string theme_id = "suborno-dark";
    bool show_candidate_window = true;
    bool next_word_prediction_enabled = false;
    CandidateOrientation candidate_orientation = CandidateOrientation::vertical;
    bool candidate_fixed_height = true;
    bool use_os_default_bangla_font = true;
    bool enable_sliding_effect = true;
    bool follow_caret = true;
    bool follow_mouse_pointer_if_caret_tracking_fails = true;
};

struct TopbarSettings {
    bool idle_dimming_enabled = true;
    std::uint32_t idle_opacity_percent = 50;
    std::uint32_t idle_time_seconds = 10;
    bool remember_position = true;
};

struct InputSettings {
    std::string layout_switch_shortcut = "F10";
    bool disable_encoding_shortcut = false;
    bool map_numpad_to_bangla_numbers = true;
    bool change_locale_to_bn_bd_when_layout_not_english = true;
    bool ignore_caps_lock_state_in_bangla = false;
    bool select_candidate_using_number_keys = true;
    TextEncoding preferred_encoding = TextEncoding::unicode;
};

struct EngineSettings {
    std::string active_layout_id = "suborno-phonetic";
    std::size_t candidate_limit = 16;
    bool learning_enabled = true;
    GeneralSettings general;
    InterfaceSettings interface;
    TopbarSettings topbar;
    InputSettings input;
};

enum class SettingsStatus {
    ok,
    io_error,
    invalid_format,
    unsupported_version,
};

struct SettingsLoadResult {
    EngineSettings settings;
    SettingsStatus status = SettingsStatus::ok;
    std::string error;
};

class SettingsStore {
  public:
    explicit SettingsStore(std::filesystem::path path);

    SettingsLoadResult load() const;
    SettingsStatus save(const EngineSettings& settings) const;

  private:
    std::filesystem::path path_;
};

}  // namespace suborno::settings
