#include "suborno/settings/settings.hpp"

#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

#include "persistence/atomic_file.hpp"

namespace suborno::settings {
namespace {

constexpr std::size_t kSettingsVersion = 1;

bool valid_layout_id(const std::string& layout_id) {
    return !layout_id.empty() && layout_id.find_first_of("=\r\n") == std::string::npos
           && layout_id.find('\0') == std::string::npos;
}

bool parse_uint(const std::string& value, std::size_t& result) {
    if (value.empty()) {
        return false;
    }
    std::size_t parsed = 0;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            return false;
        }
        const std::size_t digit = static_cast<std::size_t>(character - '0');
        if (parsed > (static_cast<std::size_t>(-1) - digit) / 10) {
            return false;
        }
        parsed = parsed * 10 + digit;
    }
    result = parsed;
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

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            result.status = SettingsStatus::invalid_format;
            result.error = "Malformed settings line";
            return result;
        }
        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (key == "version") {
            std::size_t parsed = 0;
            if (!parse_uint(value, parsed)) {
                result.status = SettingsStatus::invalid_format;
                result.error = "Invalid settings version";
                return result;
            }
            version = parsed;
            has_version = true;
        } else if (key == "active_layout") {
            if (!valid_layout_id(value)) {
                result.status = SettingsStatus::invalid_format;
                result.error = "Invalid active_layout";
                return result;
            }
            settings.active_layout_id = value;
            has_layout = true;
        } else if (key == "candidate_limit") {
            if (!parse_uint(value, settings.candidate_limit) || settings.candidate_limit == 0
                || settings.candidate_limit > 100) {
                result.status = SettingsStatus::invalid_format;
                result.error = "Invalid candidate_limit";
                return result;
            }
            has_candidate_limit = true;
        } else if (key == "learning_enabled") {
            if (value == "true") {
                settings.learning_enabled = true;
            } else if (value == "false") {
                settings.learning_enabled = false;
            } else {
                result.status = SettingsStatus::invalid_format;
                result.error = "Invalid learning_enabled";
                return result;
            }
            has_learning = true;
        } else {
            result.status = SettingsStatus::invalid_format;
            result.error = "Unknown settings key";
            return result;
        }
    }

    if (!has_version) {
        result.status = SettingsStatus::invalid_format;
        result.error = "Missing settings version";
        return result;
    }
    if (version != kSettingsVersion) {
        result.status = SettingsStatus::unsupported_version;
        result.error = "Unsupported settings version";
        return result;
    }
    if (!has_layout || !has_candidate_limit || !has_learning) {
        result.status = SettingsStatus::invalid_format;
        result.error = "Missing required settings";
        return result;
    }

    result.settings = std::move(settings);
    return result;
}

SettingsStatus SettingsStore::save(const EngineSettings& settings) const {
    if (!valid_layout_id(settings.active_layout_id) || settings.candidate_limit == 0
        || settings.candidate_limit > 100) {
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
           << "learning_enabled=" << (settings.learning_enabled ? "true" : "false") << '\n';
    if (!output) {
        return SettingsStatus::io_error;
    }

    if (suborno::persistence::write_file_atomically(path_, output.str())) {
        return SettingsStatus::io_error;
    }
    return SettingsStatus::ok;
}

}  // namespace suborno::settings
