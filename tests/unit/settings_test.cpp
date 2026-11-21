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

    EXPECT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    const auto loaded = store.load();
    EXPECT_EQ(loaded.status, suborno::settings::SettingsStatus::ok);
    EXPECT_EQ(loaded.settings.active_layout_id, settings.active_layout_id);
    EXPECT_EQ(loaded.settings.candidate_limit, settings.candidate_limit);
    EXPECT_FALSE(loaded.settings.learning_enabled);

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
