#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace suborno::settings {

struct EngineSettings {
    std::string active_layout_id = "suborno-phonetic";
    std::size_t candidate_limit = 16;
    bool learning_enabled = true;
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
