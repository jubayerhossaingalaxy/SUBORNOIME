#include "suborno/core/engine.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <system_error>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layouts/khipro/khipro_layout.hpp"
#include "suborno/layouts/national/national_layout.hpp"
#include "suborno/layouts/phonetic/avro_phonetic_layout.hpp"
#include "suborno/layouts/phonetic/borno_phonetic_layout.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"
#include "suborno/layouts/probhat/probhat_layout.hpp"
#include "suborno/settings/settings.hpp"

namespace suborno::core {

Engine::Engine() {
    layouts_.register_layout("suborno-phonetic",
                             std::make_unique<layouts::SubornoPhoneticLayout>(),
                             layout::LayoutMetadata{"suborno-phonetic", "Suborno Phonetic", "1"});
    layouts_.register_layout("borno-phonetic",
                             std::make_unique<layouts::BornoPhoneticLayout>(),
                             layout::LayoutMetadata{"borno-phonetic", "Borno Phonetic", "1"});
    layouts_.register_layout("avro-phonetic",
                             std::make_unique<layouts::AvroPhoneticLayout>(),
                             layout::LayoutMetadata{"avro-phonetic", "Avro Phonetic", "1"});
    layouts_.register_layout("khipro",
                             std::make_unique<layouts::KhiproLayout>(),
                             layout::LayoutMetadata{"khipro", "Khipro Classic", "1"});
    layouts_.register_layout("probhat",
                             std::make_unique<layouts::ProbhatLayout>(),
                             layout::LayoutMetadata{"probhat", "Probhat", "1"});
    layouts_.register_layout(
        "national",
        std::make_unique<layouts::NationalLayout>(),
        layout::LayoutMetadata{"national", "National (draft phonetic prototype)", "0.1"});
}

Engine::Engine(const EngineConfig& config)
    : Engine() {
    if (!config.settings_path.empty()) {
        settings_store_ = std::make_unique<settings::SettingsStore>(config.settings_path);
        const settings::SettingsLoadResult result = settings_store_->load();
        if (result.status == settings::SettingsStatus::ok) {
            current_settings_ = result.settings;
            const std::string_view restored_layout =
                current_settings_.general.remember_last_input_layout
                    ? std::string_view(current_settings_.active_layout_id)
                    : std::string_view("suborno-phonetic");
            if (!layouts_.set_active(restored_layout)) {
                current_settings_.active_layout_id = std::string(layouts_.active_id());
            } else {
                current_settings_.active_layout_id = std::string(restored_layout);
            }
        }
    }
    if (!config.dictionary_path.empty()) {
        const std::u8string dictionary_path = config.dictionary_path.u8string();
        dictionary_ = std::make_unique<dictionary::Dictionary>(std::string(
            reinterpret_cast<const char*>(dictionary_path.data()), dictionary_path.size()));
        if (!config.dictionary_word_lists.empty() || !config.dictionary_corpus_version.empty()) {
            if (config.dictionary_word_lists.empty() || config.dictionary_corpus_version.empty()) {
                throw std::invalid_argument("The bundled dictionary configuration is incomplete");
            }
            dictionary_->import_word_lists_if_needed(config.dictionary_word_lists,
                                                      config.dictionary_corpus_version);
        }
    }
}

Engine::~Engine() = default;

bool Engine::register_layout(std::string_view id, std::unique_ptr<layout::InputLayout> layout) {
    return layouts_.register_layout(id, std::move(layout));
}

bool Engine::set_active_layout(std::string_view id) {
    return layouts_.set_active(id);
}

std::string_view Engine::active_layout_id() const noexcept {
    return layouts_.active_id();
}

layout::LayoutKind Engine::active_layout_kind() const noexcept {
    const layout::InputLayout* active_layout = layouts_.active();
    return active_layout == nullptr ? layout::LayoutKind::phonetic : active_layout->kind();
}

const layout::LayoutMetadata* Engine::layout_metadata(std::string_view id) const {
    return layouts_.find_metadata(id);
}

std::unique_ptr<layout::InputSession> Engine::create_session() {
    const layout::InputLayout* active_layout = layouts_.active();
    if (active_layout == nullptr) {
        return nullptr;
    }

    const layout::InputSessionOptions options{current_settings_.candidate_limit,
                                              current_settings_.learning_enabled};
    return active_layout->create_session(dictionary_.get(), options);
}

settings::SettingsStatus Engine::save_settings(const settings::EngineSettings& settings) {
    if (settings_store_ == nullptr) {
        return settings::SettingsStatus::io_error;
    }
    if (layouts_.find(settings.active_layout_id) == nullptr) {
        return settings::SettingsStatus::invalid_format;
    }
    const settings::SettingsStatus status = settings_store_->save(settings);
    if (status == settings::SettingsStatus::ok) {
        current_settings_ = settings;
        layouts_.set_active(settings.active_layout_id);
    }
    return status;
}

settings::SettingsLoadResult Engine::load_settings() const {
    if (settings_store_ == nullptr) {
        settings::SettingsLoadResult result;
        result.settings = current_settings_;
        result.status = settings::SettingsStatus::ok;
        return result;
    }
    settings::SettingsLoadResult result = settings_store_->load();
    if (result.status == settings::SettingsStatus::ok
        && layouts_.find(result.settings.active_layout_id) == nullptr) {
        result.settings.active_layout_id = std::string(layouts_.active_id());
    }
    return result;
}

settings::SettingsLoadResult Engine::reload_settings() {
    settings::SettingsLoadResult result = load_settings();
    if (result.status != settings::SettingsStatus::ok) {
        return result;
    }

    if (!layouts_.set_active(result.settings.active_layout_id)) {
        result.settings.active_layout_id = std::string(layouts_.active_id());
    }
    current_settings_ = result.settings;
    return result;
}

const settings::EngineSettings& Engine::settings() const noexcept {
    return current_settings_;
}

dictionary::Dictionary* Engine::dictionary() noexcept {
    return dictionary_.get();
}

const dictionary::Dictionary* Engine::dictionary() const noexcept {
    return dictionary_.get();
}

}  // namespace suborno::core
