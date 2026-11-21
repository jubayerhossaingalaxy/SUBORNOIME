#include "suborno/core/engine.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sqlite3.h>

#include "suborno/candidates/candidate_engine.hpp"
#include "suborno/candidates/language_intelligence.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/declarative_layout.hpp"
#include "suborno/layout-api/phonetic_layout.hpp"
#include "suborno/layouts/phonetic/phonetic_input_session.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"
#include "suborno/settings/settings.hpp"
#include "suborno/suborno.h"

namespace {

class TestLayout final : public suborno::layout::PhoneticLayout {
  public:
    std::string transliterate(const std::string& input) const override {
        return "test:" + input;
    }

    std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        return {{transliterate(input),
                 transliterate(input),
                 "layout:test-layout",
                 suborno::layout::CandidateSource::layout,
                 1.0,
                 "test layout"}};
    }
};

class TestStatelessLayout final : public suborno::layout::StatelessInputLayout {
  public:
    suborno::layout::LayoutKind kind() const noexcept override {
        return suborno::layout::LayoutKind::fixed;
    }

    std::string transliterate(const std::string& input) const override {
        std::string output;
        for (const char character : input) {
            output += character == 'a' ? "ক" : std::string(1, character);
        }
        return output;
    }

    std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        const std::string output = transliterate(input);
        return {{output,
                 output,
                 "layout:stateless:" + output,
                 suborno::layout::CandidateSource::layout,
                 1.0,
                 "stateless test layout"}};
    }
};

class MultiCandidateLayout final : public suborno::layout::PhoneticLayout {
  public:
    std::string transliterate(const std::string& input) const override {
        return input;
    }

    std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        return {
            {input + "-one",
             input + "-one",
             "layout:one",
             suborno::layout::CandidateSource::layout,
             1.0,
             "first"},
            {input + "-two",
             input + "-two",
             "layout:two",
             suborno::layout::CandidateSource::layout,
             0.5,
             "second"},
        };
    }
};

class CanonicalDuplicateLayout final : public suborno::layout::PhoneticLayout {
  public:
    std::string transliterate(const std::string&) const override {
        return {};
    }

    std::vector<suborno::layout::Candidate> generate_candidates(const std::string&) const override {
        return {
            {"\u09DC",
             "\u09DC",
             "layout:precomposed",
             suborno::layout::CandidateSource::layout,
             1.0,
             "precomposed"},
            {"\u09A1\u09BC",
             "\u09A1\u09BC",
             "layout:decomposed",
             suborno::layout::CandidateSource::layout,
             0.5,
             "decomposed"},
        };
    }
};

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

}  // namespace

static_assert(!std::is_copy_constructible_v<suborno::dictionary::Dictionary>);
static_assert(!std::is_copy_assignable_v<suborno::dictionary::Dictionary>);

TEST(EngineTest, RegistersDefaultPhoneticLayout) {
    suborno::core::Engine engine;
    EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
    const auto* metadata = engine.layout_metadata("suborno-phonetic");
    ASSERT_NE(metadata, nullptr);
    EXPECT_EQ(metadata->display_name, "Suborno Phonetic");

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("ami"), suborno::layouts::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "আমি");
}

TEST(EngineTest, RegistersAndSwitchesLayouts) {
    suborno::core::Engine engine;
    EXPECT_TRUE(engine.register_layout("test", std::make_unique<TestLayout>()));
    EXPECT_TRUE(engine.set_active_layout("test"));
    EXPECT_EQ(engine.active_layout_id(), "test");

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->append_utf8("x"), suborno::layouts::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "test:x");
}

TEST(EngineTest, CreatesSessionsThroughStatelessInputLayoutFactory) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.register_layout("fixed-test", std::make_unique<TestStatelessLayout>()));
    ASSERT_TRUE(engine.set_active_layout("fixed-test"));

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("a"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
}

TEST(EngineTest, RegistersAndCreatesBornoPhoneticSessions) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("borno-phonetic"));

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "আমি");
}

TEST(EngineTest, RegistersAndCreatesKhiproCompositionalSessions) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("khipro"));
    EXPECT_EQ(engine.active_layout_kind(), suborno::layout::LayoutKind::compositional);

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("kt"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক্ত");
    EXPECT_EQ(session->commit().text_utf8, "ক্ত");
}

TEST(EngineTest, RegistersAndCreatesAvroPhoneticSessions) {
    suborno::core::Engine engine;
    ASSERT_TRUE(engine.set_active_layout("avro-phonetic"));

    auto session = engine.create_session();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "আমি");
}

TEST(EngineTest, CandidateEngineAddsDictionaryExactMatches) {
    const std::string db_path = temporary_database_path("suborno_candidate_engine_test.sqlite");
    std::remove(db_path.c_str());
    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("আমি", "আমি", 10);

    suborno::layouts::SubornoPhoneticLayout layout;
    suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ami");

    ASSERT_FALSE(candidates.empty());
    EXPECT_EQ(candidates.front().text, "আমি");
    EXPECT_EQ(candidates.front().normalized_text, "আমি");
    EXPECT_FALSE(candidates.front().stable_id.empty());
    EXPECT_EQ(candidates.front().source, suborno::layout::CandidateSource::system_dictionary);
}

TEST(EngineTest, CandidateEngineDeduplicatesCanonicalText) {
    CanonicalDuplicateLayout layout;
    suborno::candidates::CandidateEngine engine(layout);

    const auto candidates = engine.make_candidates("unused");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "\u09DC");
}

TEST(EngineTest, DictionarySchemaCommitAndReset) {
    const std::string db_path = temporary_database_path("suborno_dictionary_learning_test.sqlite");
    std::remove(db_path.c_str());
    suborno::dictionary::Dictionary dictionary(db_path);

    EXPECT_EQ(dictionary.schema_version(), 3);
    dictionary.add_entry("বাংলা", "bangla", 5);
    dictionary.record_commit("বাংলা", "bangla");

    const auto matches = dictionary.find_matches("bangla");
    ASSERT_FALSE(matches.empty());
    EXPECT_EQ(matches.front(), "বাংলা");

    dictionary.clear_user_entries();
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
}

TEST(EngineTest, DictionaryRejectsInvalidSchemaVersion) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_invalid_version_test.sqlite");
    std::remove(db_path.c_str());
    EXPECT_NO_THROW({ suborno::dictionary::Dictionary dictionary(db_path); });

    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(
        sqlite3_exec(db,
                     "UPDATE dictionary_metadata SET value='1invalid' WHERE key='schema_version';",
                     nullptr,
                     nullptr,
                     nullptr),
        SQLITE_OK);
    sqlite3_close(db);

    {
        suborno::dictionary::Dictionary dictionary(db_path);
        EXPECT_THROW(dictionary.schema_version(), suborno::dictionary::DictionaryError);
    }
    std::remove(db_path.c_str());
}

TEST(EngineTest, DictionaryMigratesLegacySchema) {
    const std::string db_path = temporary_database_path("suborno_dictionary_migration_test.sqlite");
    std::remove(db_path.c_str());

    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db,
                           "CREATE TABLE dictionary_entries("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "word TEXT NOT NULL UNIQUE,"
                           "normalized TEXT NOT NULL,"
                           "frequency INTEGER NOT NULL DEFAULT 1,"
                           "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
                           "INSERT INTO dictionary_entries(word, normalized, frequency)"
                           " VALUES('বাংলা', 'bangla', 5);",
                           nullptr,
                           nullptr,
                           nullptr),
              SQLITE_OK);
    sqlite3_close(db);

    suborno::dictionary::Dictionary dictionary(db_path);
    EXPECT_EQ(dictionary.schema_version(), 3);
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
    dictionary.record_commit("বাংলা", "bangla");
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
    std::remove(db_path.c_str());
}

TEST(EngineTest, DictionaryMigratesVersionTwoLearnedWordsToAliasSchema) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_v2_to_v3_alias_test.sqlite");
    std::remove(db_path.c_str());

    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &db), SQLITE_OK);
    ASSERT_EQ(
        sqlite3_exec(db,
                     "CREATE TABLE dictionary_metadata("
                     "key TEXT PRIMARY KEY, value TEXT NOT NULL);"
                     "INSERT INTO dictionary_metadata(key, value) VALUES('schema_version', '2');"
                     "CREATE TABLE dictionary_entries("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                     "word TEXT NOT NULL UNIQUE,"
                     "normalized TEXT NOT NULL,"
                     "frequency INTEGER NOT NULL DEFAULT 1,"
                     "source TEXT NOT NULL DEFAULT 'system',"
                     "user_frequency INTEGER NOT NULL DEFAULT 0,"
                     "user_pinned INTEGER NOT NULL DEFAULT 0,"
                     "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
                     "INSERT INTO dictionary_entries("
                     "word, normalized, frequency, source, user_frequency)"
                     " VALUES('বাংলা', 'bang', 10, 'system', 3);",
                     nullptr,
                     nullptr,
                     nullptr),
        SQLITE_OK);
    sqlite3_close(db);

    {
        suborno::dictionary::Dictionary dictionary(db_path);
        EXPECT_EQ(dictionary.schema_version(), 3);
        EXPECT_EQ(dictionary.find_matches("bang"), std::vector<std::string>{"বাংলা"});
        EXPECT_EQ(dictionary.find_prefix_matches("ban"), std::vector<std::string>{"বাংলা"});
        EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 3);
    }

    std::remove(db_path.c_str());
}

TEST(EngineTest, DictionaryRejectsCorruptDatabase) {
    const std::string db_path = temporary_database_path("suborno_dictionary_corrupt_test.sqlite");
    std::remove(db_path.c_str());
    {
        std::ofstream output(db_path, std::ios::binary);
        output << "not a sqlite database";
    }

    EXPECT_THROW(suborno::dictionary::Dictionary dictionary(db_path),
                 suborno::dictionary::DictionaryError);
    std::remove(db_path.c_str());
}

TEST(EngineTest, DictionaryPrefixLookupAndReset) {
    const std::string db_path = temporary_database_path("suborno_dictionary_prefix_test.sqlite");
    std::remove(db_path.c_str());
    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 5);
    dictionary.add_entry("বাঙালি", "bangali", 3);

    const auto matches = dictionary.words_for_prefix("বা");
    ASSERT_FALSE(matches.empty());
    dictionary.clear_user_entries();

    std::remove(db_path.c_str());
}

TEST(EngineTest, EngineConfigPersistsSettingsAndOwnsDictionary) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_settings.conf";
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path()
        / std::filesystem::path(u8"suborno_engine_dictionary_\u09AC.sqlite");
    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);

    {
        suborno::core::EngineConfig config;
        config.settings_path = settings_path;
        config.dictionary_path = dictionary_path;
        suborno::core::Engine engine(config);

        suborno::settings::EngineSettings settings = engine.settings();
        settings.candidate_limit = 24;
        settings.learning_enabled = false;
        EXPECT_EQ(engine.save_settings(settings), suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(engine.settings().candidate_limit, 24U);
        ASSERT_NE(engine.dictionary(), nullptr);

        engine.dictionary()->add_entry("আমি", "ami", 3);
        EXPECT_FALSE(engine.dictionary()->find_matches("ami").empty());
    }

    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, ReloadsSettingsChangedByTheDesktopSettingsApplication) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_reload_settings_test.conf";
    std::filesystem::remove(settings_path);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_EQ(engine.active_layout_id(), "suborno-phonetic");

        suborno::settings::EngineSettings external = engine.settings();
        external.active_layout_id = "khipro";
        external.candidate_limit = 7;
        external.input.ignore_caps_lock_state_in_bangla = true;
        ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(external),
                  suborno::settings::SettingsStatus::ok);

        const auto reloaded = engine.reload_settings();
        ASSERT_EQ(reloaded.status, suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(engine.active_layout_id(), "khipro");
        EXPECT_EQ(engine.settings().candidate_limit, 7U);
        EXPECT_TRUE(engine.settings().input.ignore_caps_lock_state_in_bangla);
    }

    std::filesystem::remove(settings_path);
}

TEST(EngineTest, RestoresKnownLayoutAndFallsBackForUnknownLayout) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_layout_restore.conf";
    std::filesystem::remove(settings_path);

    suborno::settings::EngineSettings settings;
    settings.active_layout_id = "probhat";
    suborno::settings::SettingsStore store(settings_path);
    ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    {
        suborno::core::Engine engine(config);
        EXPECT_EQ(engine.active_layout_id(), "probhat");
        EXPECT_EQ(engine.settings().active_layout_id, "probhat");

        settings.active_layout_id = "missing-layout";
        ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
        EXPECT_EQ(engine.load_settings().settings.active_layout_id, "probhat");
        EXPECT_EQ(engine.save_settings(settings),
                  suborno::settings::SettingsStatus::invalid_format);
    }

    settings.active_layout_id = "missing-layout";
    ASSERT_EQ(store.save(settings), suborno::settings::SettingsStatus::ok);
    {
        suborno::core::Engine engine(config);
        EXPECT_EQ(engine.active_layout_id(), "suborno-phonetic");
        EXPECT_EQ(engine.settings().active_layout_id, "suborno-phonetic");
    }

    std::filesystem::remove(settings_path);
}

TEST(EngineTest, AppliesCandidateLimitAndLearningSettingToSessions) {
    const std::filesystem::path settings_path =
        std::filesystem::temp_directory_path() / "suborno_engine_session_settings.conf";
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_engine_session_dictionary.sqlite";
    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);

    suborno::settings::EngineSettings settings;
    settings.candidate_limit = 1;
    settings.learning_enabled = false;
    ASSERT_EQ(suborno::settings::SettingsStore(settings_path).save(settings),
              suborno::settings::SettingsStatus::ok);

    suborno::core::EngineConfig config;
    config.settings_path = settings_path;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        engine.dictionary()->add_user_word("আমি", "আমি");
        engine.dictionary()->add_entry("আমিও", "আমি", 3);
        engine.dictionary()->add_entry("আমিই", "আমি", 2);
        ASSERT_EQ(engine.dictionary()->find_matches("আমি").size(), 3U);

        auto session = engine.create_session();
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ami"), suborno::layouts::InputStatus::updated);
        EXPECT_EQ(session->snapshot().candidates.size(), 1U);
        EXPECT_EQ(session->commit_candidate(1).status,
                  suborno::layouts::InputStatus::candidate_out_of_range);
        ASSERT_EQ(session->commit().status, suborno::layouts::InputStatus::updated);
        EXPECT_EQ(engine.dictionary()->get_user_frequency("আমি"), 1);
    }

    std::filesystem::remove(settings_path);
    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, CandidateLimitAppliesWithoutDictionary) {
    MultiCandidateLayout layout;
    suborno::layouts::PhoneticInputSession session(
        layout, suborno::layouts::InputSessionOptions{.candidate_limit = 1});

    ASSERT_EQ(session.append_utf8("ami"), suborno::layouts::InputStatus::updated);
    EXPECT_EQ(session.snapshot().candidates.size(), 1U);
    EXPECT_EQ(session.commit_candidate(1).status,
              suborno::layouts::InputStatus::candidate_out_of_range);
}

TEST(EngineTest, SessionCandidatesIncludePhoneticDictionaryMatches) {
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_session_phonetic_dictionary.sqlite";
    std::filesystem::remove(dictionary_path);

    suborno::core::EngineConfig config;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        engine.dictionary()->add_entry("আমি", "ami", 5);
        auto session = engine.create_session();
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ami"), suborno::layouts::InputStatus::updated);

        const auto snapshot = session->snapshot();
        ASSERT_FALSE(snapshot.candidates.empty());
        EXPECT_EQ(snapshot.candidates.front().text, "আমি");
        EXPECT_EQ(snapshot.candidates.front().rationale, "input dictionary match");
    }

    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, PhoneticSessionLearnsTypedInputAsSearchableDictionaryAlias) {
    const std::filesystem::path dictionary_path =
        std::filesystem::temp_directory_path() / "suborno_phonetic_session_alias_learning.sqlite";
    std::filesystem::remove(dictionary_path);

    suborno::core::EngineConfig config;
    config.dictionary_path = dictionary_path;
    {
        suborno::core::Engine engine(config);
        ASSERT_NE(engine.dictionary(), nullptr);
        engine.dictionary()->add_entry("আমি", "আমি", 10);

        ASSERT_TRUE(engine.set_active_layout("suborno-phonetic"));
        auto session = engine.create_session();
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ami"), suborno::layout::InputStatus::updated);

        const auto committed = session->commit();
        EXPECT_EQ(committed.text_utf8, "আমি");
        EXPECT_EQ(engine.dictionary()->find_matches("আমি"),
                  std::vector<std::string>{"আমি"});
        EXPECT_EQ(engine.dictionary()->find_matches("ami"),
                  std::vector<std::string>{"আমি"});
        EXPECT_EQ(engine.dictionary()->find_prefix_matches("am"),
                  std::vector<std::string>{"আমি"});
    }

    std::filesystem::remove(dictionary_path);
}

TEST(EngineTest, CommitUsesSelectedCandidate) {
    MultiCandidateLayout layout;
    suborno::layouts::PhoneticInputSession session(
        layout, suborno::layouts::InputSessionOptions{.candidate_limit = 2});

    ASSERT_EQ(session.append_utf8("ami"), suborno::layouts::InputStatus::updated);
    ASSERT_TRUE(session.select_candidate(1));
    EXPECT_EQ(session.commit().text_utf8, "ami-two");
}

TEST(EngineTest, LoadsDeclarativeLayoutMapping) {
    const std::filesystem::path mapping =
        std::filesystem::temp_directory_path() / "suborno_layout_test.tsv";
    std::ofstream output(mapping, std::ios::trunc);
    output << "# test mapping\n";
    output << "a\tআ\n";
    output << "k\tক\n";
    output.close();

    suborno::layout::DeclarativeLayout layout("test-layout", mapping);
    ASSERT_TRUE(layout.load());
    EXPECT_EQ(layout.transliterate("ka"), "কআ");
    const auto candidates = layout.generate_candidates("ka");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().stable_id, "layout:test-layout:কআ");

    std::ofstream invalid_output(mapping, std::ios::trunc);
    invalid_output << "a\tআ\n";
    invalid_output << "a\tঅ\n";
    invalid_output.close();
    EXPECT_FALSE(layout.load());
    EXPECT_EQ(layout.transliterate("ka"), "কআ");

    std::filesystem::remove(mapping);
}

TEST(EngineTest, DeterministicLanguageIntelligence) {
    const std::string db_path =
        temporary_database_path("suborno_language_intelligence_test.sqlite");
    std::remove(db_path.c_str());
    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 5);
    suborno::candidates::LanguageIntelligence intelligence(dictionary);

    EXPECT_TRUE(intelligence.is_known_word("bangla"));
    EXPECT_EQ(intelligence.correct_or_original("bangla"), "বাংলা");
    EXPECT_EQ(intelligence.correct_or_original("unknown"), "unknown");
    EXPECT_FALSE(intelligence.suggest_prefix("বা").empty());
    std::remove(db_path.c_str());
}

TEST(EngineTest, CApiCreatesAndCommitsSession) {
    SUBORNO_Engine* engine = suborno_engine_create();
    ASSERT_NE(engine, nullptr);
    SUBORNO_Session* session = suborno_session_create(engine);
    ASSERT_NE(session, nullptr);

    EXPECT_GE(suborno_session_append_utf8(session, "ami"), 0);
    char buffer[256];
    EXPECT_GE(suborno_session_snapshot(session, buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "আমি");

    std::size_t candidate_count = 0;
    EXPECT_GE(suborno_session_candidate_count(session, &candidate_count), 0);
    ASSERT_GT(candidate_count, 0U);
    char candidate[256];
    EXPECT_GE(suborno_session_candidate_snapshot(session, 0, candidate, sizeof(candidate)), 0);
    EXPECT_STREQ(candidate, "আমি");
    EXPECT_EQ(
        suborno_session_candidate_snapshot(session, candidate_count, candidate, sizeof(candidate)),
        SUBORNO_STATUS_CANDIDATE_OUT_OF_RANGE);
    EXPECT_EQ(suborno_session_select_candidate(session, 0), SUBORNO_STATUS_UPDATED);

    char* committed = nullptr;
    std::size_t committed_size = 0;
    EXPECT_GE(suborno_session_commit(session, &committed, &committed_size), 0);
    ASSERT_NE(committed, nullptr);
    EXPECT_EQ(std::string(committed), "আমি");
    suborno_string_free(committed);

    suborno_session_destroy(session);
    suborno_engine_destroy(engine);
}

TEST(EngineTest, CApiReportsInputAndArgumentErrors) {
    EXPECT_EQ(suborno_session_append_utf8(nullptr, "ami"), SUBORNO_ERROR_INVALID_ARGUMENT);

    SUBORNO_Engine* engine = suborno_engine_create();
    ASSERT_NE(engine, nullptr);
    SUBORNO_Session* session = suborno_session_create(engine);
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(suborno_session_append_utf8(session, "\xFF"), SUBORNO_STATUS_INVALID_UTF8);
    ASSERT_EQ(suborno_session_append_utf8(session, "ami"), SUBORNO_STATUS_UPDATED);

    char short_buffer[1] = {'x'};
    EXPECT_EQ(suborno_session_snapshot(session, short_buffer, sizeof(short_buffer)),
              SUBORNO_ERROR_BUFFER_TOO_SMALL);
    EXPECT_EQ(short_buffer[0], '\0');

    std::size_t candidate_count = 0;
    ASSERT_EQ(suborno_session_candidate_count(session, &candidate_count), SUBORNO_STATUS_UPDATED);
    ASSERT_GT(candidate_count, 0U);
    short_buffer[0] = 'x';
    EXPECT_EQ(suborno_session_candidate_snapshot(session, 0, short_buffer, sizeof(short_buffer)),
              SUBORNO_ERROR_BUFFER_TOO_SMALL);
    EXPECT_EQ(short_buffer[0], '\0');

    char marker = '\0';
    char* committed = &marker;
    std::size_t committed_size = 99;
    EXPECT_EQ(suborno_session_commit(nullptr, &committed, &committed_size),
              SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);
    EXPECT_EQ(committed_size, 0U);

    committed = &marker;
    EXPECT_EQ(suborno_session_commit(session, &committed, nullptr), SUBORNO_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(committed, nullptr);

    suborno_session_destroy(session);
    suborno_engine_destroy(engine);
}
