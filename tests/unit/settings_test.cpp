#include "suborno/settings/settings.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

TEST(SettingsTest, SavesAndLoadsVersionedSettings) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_test.conf";
    std::filesystem::remove(path);

    suborno::settings::SettingsStore store(path);
    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "suborno-phonetic";
    settings.candidate_limit = 24;
    settings.learning_enabled = false;
    settings.general.remember_last_input_layout = false;
    settings.general.start_with_windows = true;
    settings.general.start_mode = suborno::settings::StartMode::system_tray;
    settings.general.switching_mode = suborno::settings::SwitchingMode::global;
    settings.interface.theme_id = "suborno-glass";
    settings.interface.show_candidate_window = false;
    settings.interface.next_word_prediction_enabled = true;
    settings.interface.candidate_orientation = suborno::settings::CandidateOrientation::horizontal;
    settings.interface.candidate_fixed_height = false;
    settings.interface.use_os_default_bangla_font = false;
    settings.interface.enable_sliding_effect = false;
    settings.interface.follow_caret = false;
    settings.interface.follow_mouse_pointer_if_caret_tracking_fails = false;
    settings.topbar.idle_dimming_enabled = false;
    settings.topbar.idle_opacity_percent = 35;
    settings.topbar.idle_time_seconds = 45;
    settings.topbar.remember_position = false;
    settings.input.layout_switch_shortcut = "Alt+Shift";
    settings.input.disable_encoding_shortcut = true;
    settings.input.map_numpad_to_bangla_numbers = false;
    settings.input.change_locale_to_bn_bd_when_layout_not_english = false;
    settings.input.ignore_caps_lock_state_in_bangla = true;
    settings.input.select_candidate_using_number_keys = false;
    settings.input.preferred_encoding = suborno::settings::TextEncoding::ansi;

    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, settings.active_layout_id);
    EXPECT_EQ(loaded.settings.candidate_limit, settings.candidate_limit);
    EXPECT_FALSE(loaded.settings.learning_enabled);
    EXPECT_FALSE(loaded.settings.general.remember_last_input_layout);
    EXPECT_TRUE(loaded.settings.general.start_with_windows);
    EXPECT_EQ(loaded.settings.general.start_mode, suborno::settings::StartMode::system_tray);
    EXPECT_EQ(loaded.settings.general.switching_mode, suborno::settings::SwitchingMode::global);
    EXPECT_EQ(loaded.settings.interface.theme_id, "suborno-glass");
    EXPECT_FALSE(loaded.settings.interface.show_candidate_window);
    EXPECT_TRUE(loaded.settings.interface.next_word_prediction_enabled);
    EXPECT_EQ(loaded.settings.interface.candidate_orientation,
              suborno::settings::CandidateOrientation::horizontal);
    EXPECT_FALSE(loaded.settings.interface.candidate_fixed_height);
    EXPECT_FALSE(loaded.settings.interface.use_os_default_bangla_font);
    EXPECT_FALSE(loaded.settings.interface.enable_sliding_effect);
    EXPECT_FALSE(loaded.settings.interface.follow_caret);
    EXPECT_FALSE(loaded.settings.interface.follow_mouse_pointer_if_caret_tracking_fails);
    EXPECT_FALSE(loaded.settings.topbar.idle_dimming_enabled);
    EXPECT_EQ(loaded.settings.topbar.idle_opacity_percent, 35U);
    EXPECT_EQ(loaded.settings.topbar.idle_time_seconds, 45U);
    EXPECT_FALSE(loaded.settings.topbar.remember_position);
    EXPECT_EQ(loaded.settings.input.layout_switch_shortcut, "Alt+Shift");
    EXPECT_TRUE(loaded.settings.input.disable_encoding_shortcut);
    EXPECT_FALSE(loaded.settings.input.map_numpad_to_bangla_numbers);
    EXPECT_FALSE(loaded.settings.input.change_locale_to_bn_bd_when_layout_not_english);
    EXPECT_TRUE(loaded.settings.input.ignore_caps_lock_state_in_bangla);
    EXPECT_FALSE(loaded.settings.input.select_candidate_using_number_keys);
    EXPECT_EQ(loaded.settings.input.preferred_encoding, suborno::settings::TextEncoding::ansi);

    std::filesystem::remove(path);
}

TEST(SettingsTest, MissingFileUsesDefaults) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_missing_test.conf";
    std::filesystem::remove(path);

    const auto loaded = suborno::settings::SettingsStore(path).load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, "suborno-phonetic");
    EXPECT_EQ(loaded.settings.candidate_limit, 16U);
    EXPECT_TRUE(loaded.settings.learning_enabled);
}

TEST(SettingsTest, MigratesLegacyVersionOneSettingsToCurrentDefaults) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_v1_migration_test.conf";
    {
        std::ofstream output(path, std::ios::trunc);
        output << "version=1\n"
               << "active_layout=probhat\n"
               << "candidate_limit=12\n"
               << "learning_enabled=false\n";
    }

    suborno::settings::SettingsStore store(path);
    const auto loaded = store.load();
    ASSERT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, "probhat");
    EXPECT_EQ(loaded.settings.candidate_limit, 12U);
    EXPECT_FALSE(loaded.settings.learning_enabled);
    EXPECT_EQ(loaded.settings.interface.theme_id, "suborno-dark");
    EXPECT_EQ(loaded.settings.topbar.idle_opacity_percent, 50U);
    EXPECT_EQ(loaded.settings.input.layout_switch_shortcut, "F10");
    EXPECT_EQ(loaded.settings.input.preferred_encoding, suborno::settings::TextEncoding::unicode);

    EXPECT_EQ(store.save(loaded.settings), suborno::settings::SettingsStatus::ok);
    std::ifstream migrated(path);
    std::string first_line;
    ASSERT_TRUE(std::getline(migrated, first_line));
    EXPECT_EQ(first_line, "version=2");
    std::filesystem::remove(path);
}

TEST(SettingsTest, RejectsInvalidDesktopRangesShortcutsAndDuplicateKeys) {
    suborno::settings::EngineSettings invalid;
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_invalid_desktop_test.conf";

    invalid.topbar.idle_opacity_percent = 101;
    EXPECT_EQ(suborno::settings::SettingsStore(path).save(invalid),
              suborno::settings::SettingsStatus::invalid_format);
    invalid = {};
    invalid.topbar.idle_time_seconds = 0;
    EXPECT_EQ(suborno::settings::SettingsStore(path).save(invalid),
              suborno::settings::SettingsStatus::invalid_format);
    invalid = {};
    invalid.input.layout_switch_shortcut = "F99";
    EXPECT_EQ(suborno::settings::SettingsStore(path).save(invalid),
              suborno::settings::SettingsStatus::invalid_format);
    invalid = {};
    invalid.interface.theme_id = "theme=invalid";
    EXPECT_EQ(suborno::settings::SettingsStore(path).save(invalid),
              suborno::settings::SettingsStatus::invalid_format);

    {
        std::ofstream output(path, std::ios::trunc);
        output << "version=2\n"
               << "active_layout=suborno-phonetic\n"
               << "active_layout=probhat\n"
               << "candidate_limit=16\n"
               << "learning_enabled=true\n";
    }
    EXPECT_EQ(suborno::settings::SettingsStore(path).load().status,
              suborno::settings::SettingsStatus::invalid_format);
    std::filesystem::remove(path);
}

TEST(SettingsTest, RejectsMalformedOrUnsupportedFiles) {
    const std::filesystem::path malformed =
        std::filesystem::temp_directory_path() / "suborno_settings_malformed_test.conf";
    std::ofstream malformed_output(malformed, std::ios::trunc);
    malformed_output << "not-a-settings-line\n";
    malformed_output.close();

    const auto malformed_result = suborno::settings::SettingsStore(malformed).load();
    EXPECT_EQ(malformed_result.status, suborno::settings::SettingsStatus::invalid_format);

    const std::filesystem::path unsupported =
        std::filesystem::temp_directory_path() / "suborno_settings_unsupported_test.conf";
    std::ofstream unsupported_output(unsupported, std::ios::trunc);
    unsupported_output << "version=99\n"
                       << "active_layout=suborno-phonetic\n"
                       << "candidate_limit=16\n"
                       << "learning_enabled=true\n";
    unsupported_output.close();

    const auto unsupported_result = suborno::settings::SettingsStore(unsupported).load();
    EXPECT_EQ(unsupported_result.status, suborno::settings::SettingsStatus::unsupported_version);

    std::filesystem::remove(malformed);
    std::filesystem::remove(unsupported);
}

TEST(SettingsTest, RejectsOverflowingVersionAndInvalidLayoutIds) {
    const std::filesystem::path version_path =
        std::filesystem::temp_directory_path() / "suborno_settings_overflow_version_test.conf";
    {
        std::ofstream output(version_path, std::ios::trunc);
        output << "version=4294967297\n"
               << "active_layout=suborno-phonetic\n"
               << "candidate_limit=16\n"
               << "learning_enabled=true\n";
    }

    const auto version_result = suborno::settings::SettingsStore(version_path).load();
    EXPECT_NE(version_result.status, suborno::settings::SettingsStatus::ok);

    suborno::settings::EngineSettings invalid_settings;
    invalid_settings.active_layout_id = "layout\ncandidate_limit=100";
    const std::filesystem::path invalid_layout_path =
        std::filesystem::temp_directory_path() / "suborno_settings_invalid_layout_test.conf";
    EXPECT_EQ(suborno::settings::SettingsStore(invalid_layout_path).save(invalid_settings),
              suborno::settings::SettingsStatus::invalid_format);
    EXPECT_FALSE(std::filesystem::exists(invalid_layout_path));

    std::filesystem::remove(version_path);
}

TEST(SettingsTest, ReplacesExistingSettingsFile) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_replace_test.conf";
    std::filesystem::remove(path);
    suborno::settings::SettingsStore store(path);

    suborno::settings::EngineSettings settings;
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    settings.candidate_limit = 32;
    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);

    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.candidate_limit, 32U);
    std::filesystem::remove(path);
}

TEST(SettingsTest, AcceptsTheKhiproCompositionalLayoutId) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "suborno_settings_khipro_layout_test.conf";
    std::filesystem::remove(path);

    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "khipro";
    const suborno::settings::SettingsStore store(path);

    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(store.load().settings.active_layout_id, "khipro");

    std::filesystem::remove(path);
}
