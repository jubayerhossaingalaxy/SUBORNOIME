#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "suborno/layout-api/input_session.hpp"
#include "suborno/layout-api/layout_registry.hpp"
#include "suborno/settings/settings.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::core {

struct EngineConfig {
    std::filesystem::path settings_path;
    std::filesystem::path dictionary_path;
    std::vector<std::filesystem::path> dictionary_word_lists;
    std::string dictionary_corpus_version;
};

class Engine {
  public:
    Engine();
    explicit Engine(const EngineConfig& config);
    ~Engine();

    bool register_layout(std::string_view id, std::unique_ptr<layout::InputLayout> layout);
    bool set_active_layout(std::string_view id);
    std::string_view active_layout_id() const noexcept;
    layout::LayoutKind active_layout_kind() const noexcept;
    const layout::LayoutMetadata* layout_metadata(std::string_view id) const;

    std::unique_ptr<layout::InputSession> create_session();

    settings::SettingsStatus save_settings(const settings::EngineSettings& settings);
    settings::SettingsLoadResult load_settings() const;
    settings::SettingsLoadResult reload_settings();
    const settings::EngineSettings& settings() const noexcept;
    dictionary::Dictionary* dictionary() noexcept;
    const dictionary::Dictionary* dictionary() const noexcept;

  private:
    layout::LayoutRegistry layouts_;
    settings::EngineSettings current_settings_;
    std::unique_ptr<settings::SettingsStore> settings_store_;
    std::unique_ptr<dictionary::Dictionary> dictionary_;
};

}  // namespace suborno::core
