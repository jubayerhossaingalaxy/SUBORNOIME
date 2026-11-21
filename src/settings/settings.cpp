#include "suborno/settings/settings.hpp"

#include <array>
#include <fstream>
#include <limits>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>

#include "persistence/atomic_file.hpp"

namespace suborno::settings {
namespace {

constexpr std::size_t kSettingsVersion = 2;
constexpr std::size_t kLegacySettingsVersion = 1;
constexpr std::size_t kMaximumSettingsLineLength = 4096;

bool valid_value_string(const std::string& value) {
    return !value.empty() && value.find_first_of("=\r\n") == std::string::npos
           && value.find('\0') == std::string::npos;
}

bool valid_layout_id(const std::string& layout_id) {
    return valid_value_string(layout_id);
}

bool valid_theme_id(const std::string& theme_id) {
    if (!valid_value_string(theme_id)) {
        return false;
    }
    for (const unsigned char character : theme_id) {
        const bool allowed = (character >= 'a' && character <= 'z')
                             || (character >= '0' && character <= '9')
                             || character == '-' || character == '_';
        if (!allowed) {
            return false;
        }
    }
    return true;
}

bool parse_uint(std::string_view value, std::size_t& result) {
    if (value.empty()) {
        return false;
    }

    std::size_t parsed = 0;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            return false;
        }
        const std::size_t digit = static_cast<std::size_t>(character - '0');
        if (parsed > (std::numeric_limits<std::size_t>::max() - digit) / 10) {
            return false;
        }
        parsed = parsed * 10 + digit;
    }

    result = parsed;
    return true;
}

bool parse_bool(std::string_view value, bool& result) {
    if (value == "true") {
        result = true;
        return true;
    }
    if (value == "false") {
        result = false;
        return true;
    }
    return false;
}

std::string_view start_mode_name(StartMode value) {
    switch (value) {
    case StartMode::last_state:
        return "last-state";
    case StartMode::topbar:
        return "topbar";
    case StartMode::system_tray:
        return "system-tray";
    case StartMode::main_window:
        return "main-window";
    }
    return {};
}

bool parse_start_mode(std::string_view value, StartMode& result) {
    if (value == "last-state") {
        result = StartMode::last_state;
    } else if (value == "topbar") {
        result = StartMode::topbar;
    } else if (value == "system-tray") {
        result = StartMode::system_tray;
    } else if (value == "main-window") {
        result = StartMode::main_window;
    } else {
        return false;
    }
    return true;
}

std::string_view switching_mode_name(SwitchingMode value) {
    switch (value) {
    case SwitchingMode::per_application_window:
        return "per-application-window";
    case SwitchingMode::global:
        return "global";
    }
    return {};
}

bool parse_switching_mode(std::string_view value, SwitchingMode& result) {
    if (value == "per-application-window") {
        result = SwitchingMode::per_application_window;
    } else if (value == "global") {
        result = SwitchingMode::global;
    } else {
        return false;
    }
    return true;
}

std::string_view orientation_name(CandidateOrientation value) {
    switch (value) {
    case CandidateOrientation::vertical:
        return "vertical";
    case CandidateOrientation::horizontal:
        return "horizontal";
    }
    return {};
}

bool parse_orientation(std::string_view value, CandidateOrientation& result) {
    if (value == "vertical") {
        result = CandidateOrientation::vertical;
    } else if (value == "horizontal") {
        result = CandidateOrientation::horizontal;
    } else {
        return false;
    }
    return true;
}

std::string_view encoding_name(TextEncoding value) {
    switch (value) {
    case TextEncoding::unicode:
        return "unicode";
    case TextEncoding::ansi:
        return "ansi";
    }
    return {};
}

bool parse_encoding(std::string_view value, TextEncoding& result) {
    if (value == "unicode") {
        result = TextEncoding::unicode;
    } else if (value == "ansi") {
        result = TextEncoding::ansi;
    } else {
        return false;
    }
    return true;
}

bool valid_switching_shortcut(const std::string& shortcut) {
    constexpr std::array<std::string_view, 9> kAllowedShortcuts{
        "F1",
        "F2",
        "F3",
        "F4",
        "F10",
        "F12",
        "Ctrl+Space",
        "Alt+Shift",
        "Win+Space",
    };
    for (const std::string_view allowed : kAllowedShortcuts) {
        if (shortcut == allowed) {
            return true;
        }
    }
    return false;
}

bool validate(const EngineSettings& settings) {
    return valid_layout_id(settings.active_layout_id) && settings.candidate_limit > 0
           && settings.candidate_limit <= 100 && valid_theme_id(settings.interface.theme_id)
           && settings.topbar.idle_opacity_percent <= 100
           && settings.topbar.idle_time_seconds >= 1
           && settings.topbar.idle_time_seconds <= 3600
           && valid_switching_shortcut(settings.input.layout_switch_shortcut)
           && !start_mode_name(settings.general.start_mode).empty()
           && !switching_mode_name(settings.general.switching_mode).empty()
           && !orientation_name(settings.interface.candidate_orientation).empty()
           && !encoding_name(settings.input.preferred_encoding).empty();
}

bool parse_settings_value(EngineSettings& settings,
                          const std::string& key,
                          const std::string& value) {
    std::size_t numeric_value = 0;

    if (key == "active_layout") {
        if (!valid_layout_id(value)) {
            return false;
        }
        settings.active_layout_id = value;
    } else if (key == "candidate_limit") {
        if (!parse_uint(value, numeric_value) || numeric_value == 0 || numeric_value > 100) {
            return false;
        }
        settings.candidate_limit = numeric_value;
    } else if (key == "learning_enabled") {
        if (!parse_bool(value, settings.learning_enabled)) {
            return false;
        }
    } else if (key == "remember_last_input_layout") {
        if (!parse_bool(value, settings.general.remember_last_input_layout)) {
            return false;
        }
    } else if (key == "start_with_windows") {
        if (!parse_bool(value, settings.general.start_with_windows)) {
            return false;
        }
    } else if (key == "start_mode") {
        if (!parse_start_mode(value, settings.general.start_mode)) {
            return false;
        }
    } else if (key == "switching_mode") {
        if (!parse_switching_mode(value, settings.general.switching_mode)) {
            return false;
        }
    } else if (key == "theme_id") {
        if (!valid_theme_id(value)) {
            return false;
        }
        settings.interface.theme_id = value;
    } else if (key == "show_candidate_window") {
        if (!parse_bool(value, settings.interface.show_candidate_window)) {
            return false;
        }
    } else if (key == "next_word_prediction_enabled") {
        if (!parse_bool(value, settings.interface.next_word_prediction_enabled)) {
            return false;
        }
    } else if (key == "candidate_orientation") {
        if (!parse_orientation(value, settings.interface.candidate_orientation)) {
            return false;
        }
    } else if (key == "candidate_fixed_height") {
        if (!parse_bool(value, settings.interface.candidate_fixed_height)) {
            return false;
        }
    } else if (key == "use_os_default_bangla_font") {
        if (!parse_bool(value, settings.interface.use_os_default_bangla_font)) {
            return false;
        }
    } else if (key == "enable_sliding_effect") {
        if (!parse_bool(value, settings.interface.enable_sliding_effect)) {
            return false;
        }
    } else if (key == "follow_caret") {
        if (!parse_bool(value, settings.interface.follow_caret)) {
            return false;
        }
    } else if (key == "follow_mouse_pointer_if_caret_tracking_fails") {
        if (!parse_bool(value, settings.interface.follow_mouse_pointer_if_caret_tracking_fails)) {
            return false;
        }
    } else if (key == "idle_dimming_enabled") {
        if (!parse_bool(value, settings.topbar.idle_dimming_enabled)) {
            return false;
        }
    } else if (key == "idle_opacity_percent") {
        if (!parse_uint(value, numeric_value) || numeric_value > 100) {
            return false;
        }
        settings.topbar.idle_opacity_percent = static_cast<std::uint32_t>(numeric_value);
    } else if (key == "idle_time_seconds") {
        if (!parse_uint(value, numeric_value) || numeric_value < 1 || numeric_value > 3600) {
            return false;
        }
        settings.topbar.idle_time_seconds = static_cast<std::uint32_t>(numeric_value);
    } else if (key == "remember_topbar_position") {
        if (!parse_bool(value, settings.topbar.remember_position)) {
            return false;
        }
    } else if (key == "layout_switch_shortcut") {
        if (!valid_switching_shortcut(value)) {
            return false;
        }
        settings.input.layout_switch_shortcut = value;
    } else if (key == "disable_encoding_shortcut") {
        if (!parse_bool(value, settings.input.disable_encoding_shortcut)) {
            return false;
        }
    } else if (key == "map_numpad_to_bangla_numbers") {
        if (!parse_bool(value, settings.input.map_numpad_to_bangla_numbers)) {
            return false;
        }
    } else if (key == "change_locale_to_bn_bd_when_layout_not_english") {
        if (!parse_bool(value, settings.input.change_locale_to_bn_bd_when_layout_not_english)) {
            return false;
        }
    } else if (key == "ignore_caps_lock_state_in_bangla") {
        if (!parse_bool(value, settings.input.ignore_caps_lock_state_in_bangla)) {
            return false;
        }
    } else if (key == "select_candidate_using_number_keys") {
        if (!parse_bool(value, settings.input.select_candidate_using_number_keys)) {
            return false;
        }
    } else if (key == "preferred_encoding") {
        if (!parse_encoding(value, settings.input.preferred_encoding)) {
            return false;
        }
    } else {
        return false;
    }

    return true;
}

}  // namespace

SettingsStore::SettingsStore(std::filesystem::path path)
    : path_(std::move(path)) {}

SettingsLoadResult SettingsStore::load() const {
    SettingsLoadResult result;
    std::ifstream input(path_);
    if (!input.is_open()) {
        result.status =
            std::filesystem::exists(path_) ? SettingsStatus::io_error : SettingsStatus::ok;
        return result;
    }

    EngineSettings settings;
    std::string line;
    std::size_t version = 0;
    bool has_version = false;
    bool has_layout = false;
    bool has_candidate_limit = false;
    bool has_learning = false;
    std::unordered_set<std::string> seen_keys;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        if (line.size() > kMaximumSettingsLineLength) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Settings line exceeds the maximum supported length";
            return result;
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Malformed settings line";
            return result;
        }
        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (!seen_keys.insert(key).second) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Duplicate settings key";
            return result;
        }
        if (key == "version") {
            std::size_t parsed = 0;
            if (!parse_uint(value, parsed)) {
                result.status = SettingsStatus::invalid_format;
                result.error = "Invalid settings version";
                return result;
            }
            version = parsed;
            has_version = true;
            continue;
        }

        if (!parse_settings_value(settings, key, value)) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Invalid or unknown settings key: " + key;
            return result;
        }
        has_layout = has_layout || key == "active_layout";
        has_candidate_limit = has_candidate_limit || key == "candidate_limit";
        has_learning = has_learning || key == "learning_enabled";
    }

    if (input.bad()) {
        result.status = SettingsStatus::io_error;
        result.error = "Failed while reading settings";
        return result;
    }
    if (!has_version) {
        result.status = SettingsStatus::invalid_format;
        result.error = "Missing settings version";
        return result;
    }
    if (version != kLegacySettingsVersion && version != kSettingsVersion) {
        result.status = SettingsStatus::unsupported_version;
        result.error = "Unsupported settings version";
        return result;
    }
    if (!has_layout || !has_candidate_limit || !has_learning) {
        result.status = SettingsStatus::invalid_format;
        result.error = "Missing required engine settings";
        return result;
    }
    if (!validate(settings)) {
        result.status = SettingsStatus::invalid_format;
        result.error = "Settings values are outside the supported range";
        return result;
    }

    result.settings = std::move(settings);
    return result;
}

SettingsStatus SettingsStore::save(const EngineSettings& settings) const {
    if (!validate(settings)) {
        return SettingsStatus::invalid_format;
    }

    std::error_code error;
    const std::filesystem::path parent = path_.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            return SettingsStatus::io_error;
        }
    }

    std::ostringstream output;
    output << "version=" << kSettingsVersion << '\n'
           << "active_layout=" << settings.active_layout_id << '\n'
           << "candidate_limit=" << settings.candidate_limit << '\n'
           << "learning_enabled=" << (settings.learning_enabled ? "true" : "false") << '\n'
           << "remember_last_input_layout="
           << (settings.general.remember_last_input_layout ? "true" : "false") << '\n'
           << "start_with_windows=" << (settings.general.start_with_windows ? "true" : "false")
           << '\n'
           << "start_mode=" << start_mode_name(settings.general.start_mode) << '\n'
           << "switching_mode=" << switching_mode_name(settings.general.switching_mode) << '\n'
           << "theme_id=" << settings.interface.theme_id << '\n'
           << "show_candidate_window="
           << (settings.interface.show_candidate_window ? "true" : "false") << '\n'
           << "next_word_prediction_enabled="
           << (settings.interface.next_word_prediction_enabled ? "true" : "false") << '\n'
           << "candidate_orientation="
           << orientation_name(settings.interface.candidate_orientation) << '\n'
           << "candidate_fixed_height="
           << (settings.interface.candidate_fixed_height ? "true" : "false") << '\n'
           << "use_os_default_bangla_font="
           << (settings.interface.use_os_default_bangla_font ? "true" : "false") << '\n'
           << "enable_sliding_effect="
           << (settings.interface.enable_sliding_effect ? "true" : "false") << '\n'
           << "follow_caret=" << (settings.interface.follow_caret ? "true" : "false") << '\n'
           << "follow_mouse_pointer_if_caret_tracking_fails="
           << (settings.interface.follow_mouse_pointer_if_caret_tracking_fails ? "true" : "false")
           << '\n'
           << "idle_dimming_enabled="
           << (settings.topbar.idle_dimming_enabled ? "true" : "false") << '\n'
           << "idle_opacity_percent=" << settings.topbar.idle_opacity_percent << '\n'
           << "idle_time_seconds=" << settings.topbar.idle_time_seconds << '\n'
           << "remember_topbar_position="
           << (settings.topbar.remember_position ? "true" : "false") << '\n'
           << "layout_switch_shortcut=" << settings.input.layout_switch_shortcut << '\n'
           << "disable_encoding_shortcut="
           << (settings.input.disable_encoding_shortcut ? "true" : "false") << '\n'
           << "map_numpad_to_bangla_numbers="
           << (settings.input.map_numpad_to_bangla_numbers ? "true" : "false") << '\n'
           << "change_locale_to_bn_bd_when_layout_not_english="
           << (settings.input.change_locale_to_bn_bd_when_layout_not_english ? "true" : "false")
           << '\n'
           << "ignore_caps_lock_state_in_bangla="
           << (settings.input.ignore_caps_lock_state_in_bangla ? "true" : "false") << '\n'
           << "select_candidate_using_number_keys="
           << (settings.input.select_candidate_using_number_keys ? "true" : "false") << '\n'
           << "preferred_encoding=" << encoding_name(settings.input.preferred_encoding) << '\n';
    if (!output) {
        return SettingsStatus::io_error;
    }

    if (suborno::persistence::write_file_atomically(path_, output.str())) {
        return SettingsStatus::io_error;
    }
    return SettingsStatus::ok;
}

}  // namespace suborno::settings
