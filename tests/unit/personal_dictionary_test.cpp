#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sqlite3.h>
#include <string>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"

namespace {

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

}  // namespace

TEST(PersonalDictionaryTest, RejectsNewerSchemaWithoutMutatingTheDatabase) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_future_schema_test.sqlite");
    std::remove(db_path.c_str());

    sqlite3* database = nullptr;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &database), SQLITE_OK);
    ASSERT_NE(database, nullptr);

    const char* create_future_schema =
        "CREATE TABLE dictionary_metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL);"
        "INSERT INTO dictionary_metadata(key, value) VALUES('schema_version', '4');"
        "CREATE TABLE dictionary_entries ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "word TEXT NOT NULL UNIQUE,"
        "normalized TEXT NOT NULL,"
        "frequency INTEGER NOT NULL DEFAULT 1);";
    ASSERT_EQ(sqlite3_exec(database, create_future_schema, nullptr, nullptr, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_close(database), SQLITE_OK);

    EXPECT_THROW(
        { suborno::dictionary::Dictionary dictionary(db_path); },
        suborno::dictionary::DictionaryError);

    ASSERT_EQ(sqlite3_open(db_path.c_str(), &database), SQLITE_OK);
    ASSERT_NE(database, nullptr);

    sqlite3_stmt* version_stmt = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(database,
                                 "SELECT value FROM dictionary_metadata WHERE key='schema_version'",
                                 -1,
                                 &version_stmt,
                                 nullptr),
              SQLITE_OK);
    ASSERT_EQ(sqlite3_step(version_stmt), SQLITE_ROW);
    EXPECT_STREQ(reinterpret_cast<const char*>(sqlite3_column_text(version_stmt, 0)), "4");
    EXPECT_EQ(sqlite3_finalize(version_stmt), SQLITE_OK);

    sqlite3_stmt* column_stmt = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(
                  database, "PRAGMA table_info(dictionary_entries)", -1, &column_stmt, nullptr),
              SQLITE_OK);
    bool has_source_column = false;
    int step_result = SQLITE_OK;
    while ((step_result = sqlite3_step(column_stmt)) == SQLITE_ROW) {
        const auto* column_name = sqlite3_column_text(column_stmt, 1);
        if (column_name != nullptr
            && std::string(reinterpret_cast<const char*>(column_name)) == "source") {
            has_source_column = true;
        }
    }
    EXPECT_EQ(step_result, SQLITE_DONE);
    EXPECT_FALSE(has_source_column);
    EXPECT_EQ(sqlite3_finalize(column_stmt), SQLITE_OK);

    EXPECT_EQ(sqlite3_close(database), SQLITE_OK);
    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, OpensDatabaseAtUnicodeFilesystemPath) {
    const std::filesystem::path database_path =
        std::filesystem::temp_directory_path()
        / std::filesystem::u8path(u8"সুবর্ণ_dictionary.sqlite");
    std::error_code remove_error;
    std::filesystem::remove(database_path, remove_error);

    const std::u8string utf8_path = database_path.u8string();
    const std::string database_path_utf8(reinterpret_cast<const char*>(utf8_path.data()),
                                         utf8_path.size());

    {
        suborno::dictionary::Dictionary dictionary(database_path_utf8);
        dictionary.add_entry("বাংলা", "bangla", 1);
        EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
    }

    EXPECT_TRUE(std::filesystem::exists(database_path));
    std::filesystem::remove(database_path, remove_error);
}

TEST(PersonalDictionaryTest, AddAndListUserWords) {
    const std::string db_path = temporary_database_path("suborno_personal_add_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);

    dictionary.add_user_word("সুইডেন", "sweden");
    dictionary.add_user_word("নরওয়ে", "norway");

    const auto user_words = dictionary.list_user_words();
    ASSERT_EQ(user_words.size(), 2U);

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, GetUserFrequency) {
    const std::string db_path = temporary_database_path("suborno_personal_freq_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);

    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 0);

    dictionary.add_user_word("সুইডেন", "sweden");
    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 1);

    dictionary.add_user_word("সুইডেন", "sweden");
    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 2);

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, RemoveUserWord) {
    const std::string db_path = temporary_database_path("suborno_personal_remove_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);

    dictionary.add_user_word("সুইডেন", "sweden");
    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 1);

    dictionary.remove_user_word("সুইডেন");
    EXPECT_EQ(dictionary.get_user_frequency("সুইডেন"), 0);

    const auto user_words = dictionary.list_user_words();
    EXPECT_TRUE(user_words.empty());

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, UserWordsRankedInFindMatches) {
    const std::string db_path = temporary_database_path("suborno_personal_rank_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);

    dictionary.add_entry("বাংলা", "bangla", 5);
    dictionary.add_user_word("বাংলাদেশ", "bangladesh");

    const auto matches = dictionary.find_matches("bangla");
    ASSERT_FALSE(matches.empty());

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, PrefixSearchTreatsLikeWildcardsLiterally) {
    const std::string db_path = temporary_database_path("suborno_personal_prefix_wildcard.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("p%word", "percent", 2);
    dictionary.add_entry("pXword", "other", 1);
    dictionary.add_entry("p_word", "underscore", 1);
    dictionary.add_entry("p\\word", "backslash", 1);

    EXPECT_EQ(dictionary.words_for_prefix("p%"), std::vector<std::string>{"p%word"});
    EXPECT_EQ(dictionary.words_for_prefix("p_"), std::vector<std::string>{"p_word"});
    EXPECT_EQ(dictionary.words_for_prefix("p\\"), std::vector<std::string>{"p\\word"});

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, LimitsFindMatchesAtTheSqlQueryBoundary) {
    const std::string db_path = temporary_database_path("suborno_personal_query_limit_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "shared", 9);
    dictionary.add_entry("বাংলাদেশ", "shared", 7);
    dictionary.add_entry("বাংলাদেশি", "shared", 5);

    const auto limited = dictionary.find_matches("shared", 2);
    ASSERT_EQ(limited.size(), 2U);
    EXPECT_EQ(limited[0], "বাংলা");
    EXPECT_EQ(limited[1], "বাংলাদেশ");
    EXPECT_TRUE(dictionary.find_matches("shared", 0).empty());

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, ImportsSystemWordListsOnceAndPreservesThemWhenClearingUserData) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "suborno_system_wordlist_import_test";
    const std::filesystem::path database_path = root / "dictionary.sqlite";
    const std::filesystem::path first_list = root / "part-01.txt";
    const std::filesystem::path second_list = root / "part-02.txt";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    {
        std::ofstream first(first_list, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(first.is_open());
        first << "বাংলা\nবাংলাদেশ\n";
        std::ofstream second(second_list, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(second.is_open());
        second << "বাংলা\nনদী\n";
    }

    {
        suborno::dictionary::Dictionary dictionary(database_path.string());
        const std::vector<std::filesystem::path> word_lists{first_list, second_list};
        EXPECT_EQ(dictionary.import_word_lists_if_needed(word_lists, "test-wordlist-v1"), 3U);
        EXPECT_EQ(dictionary.find_matches("বাংলা"), std::vector<std::string>{"বাংলা"});
        EXPECT_EQ(dictionary.find_matches("বাংলাদেশ"), std::vector<std::string>{"বাংলাদেশ"});
        EXPECT_EQ(dictionary.import_word_lists_if_needed(word_lists, "test-wordlist-v1"), 0U);

        dictionary.add_user_word("নিজস্ব", "nijoshsho");
        dictionary.clear_user_entries();
        EXPECT_TRUE(dictionary.list_user_words().empty());
        EXPECT_TRUE(dictionary.find_matches("nijoshsho").empty());
        EXPECT_EQ(dictionary.find_matches("বাংলা"), std::vector<std::string>{"বাংলা"});
        EXPECT_EQ(dictionary.find_matches("নদী"), std::vector<std::string>{"নদী"});
    }

    std::filesystem::remove_all(root);
}

TEST(PersonalDictionaryTest, PinningAndRemovingSystemWordPreservesSystemDictionaryEntry) {
    const std::string db_path = temporary_database_path("suborno_personal_system_pin_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);

    dictionary.add_user_word("বাংলা", "bangla");
    ASSERT_EQ(dictionary.list_user_words(), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");

    dictionary.remove_user_word("বাংলা");
    EXPECT_TRUE(dictionary.list_user_words().empty());
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 0);

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, ClearingUserEntriesUnpinsSystemWordsAndDeletesUserOnlyWords) {
    const std::string db_path = temporary_database_path("suborno_personal_clear_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.add_user_word("বাংলা", "bangla");
    dictionary.add_user_word("নিজস্ব", "nijoshsho");
    dictionary.record_commit("বাংলা", "bangla");

    ASSERT_EQ(dictionary.list_user_words().size(), 2U);
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 2);

    dictionary.clear_user_entries();

    EXPECT_TRUE(dictionary.list_user_words().empty());
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 0);
    EXPECT_EQ(dictionary.find_matches("bangla").front(), "বাংলা");
    EXPECT_TRUE(dictionary.find_matches("nijoshsho").empty());

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, FindsCandidatesByNormalizedPhoneticPrefix) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_normalized_prefix_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.add_entry("বাংলাদেশ", "bangladesh", 8);
    dictionary.add_entry("বাংলাদেশি", "bangladeshi", 6);
    dictionary.add_entry("ভারত", "bharat", 4);

    const auto matches = dictionary.find_prefix_matches("bang", 2);
    ASSERT_EQ(matches.size(), 2U);
    EXPECT_EQ(matches[0], "বাংলা");
    EXPECT_EQ(matches[1], "বাংলাদেশ");
    EXPECT_TRUE(dictionary.find_prefix_matches("bang", 0).empty());
    EXPECT_TRUE(dictionary.find_prefix_matches("", 10).empty());

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, EscapesSqlWildcardsInNormalizedPrefixSearch) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_normalized_prefix_wildcard_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("percent", "p%word", 2);
    dictionary.add_entry("underscore", "p_word", 2);
    dictionary.add_entry("backslash", "p\\word", 2);
    dictionary.add_entry("other", "pXword", 1);

    EXPECT_EQ(dictionary.find_prefix_matches("p%", 10), std::vector<std::string>{"percent"});
    EXPECT_EQ(dictionary.find_prefix_matches("p_", 10), std::vector<std::string>{"underscore"});
    EXPECT_EQ(dictionary.find_prefix_matches("p\\", 10), std::vector<std::string>{"backslash"});

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, RecordsNewWordsAsLearnedPersonalEntries) {
    const std::string db_path =
        temporary_database_path("suborno_personal_learned_word_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.record_commit("নিজস্বশব্দ", "nijoshsho");

    EXPECT_EQ(dictionary.get_user_frequency("নিজস্বশব্দ"), 1);
    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"নিজস্বশব্দ"});

    dictionary.record_commit("নিজস্বশব্দ", "nijoshsho");
    EXPECT_EQ(dictionary.get_user_frequency("নিজস্বশব্দ"), 2);

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, StoresUserInputAsAliasWithoutOverwritingCanonicalNormalization) {
    const std::string db_path =
        temporary_database_path("suborno_dictionary_alias_preserves_canonical_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.record_commit("বাংলা", "bangla", "bang");

    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_matches("bang"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_prefix_matches("ban"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_prefix_matches("bang"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 1);
    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"বাংলা"});

    dictionary.record_commit("বাংলা", "bangla", "bngla");
    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_matches("bang"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.find_matches("bngla"), std::vector<std::string>{"বাংলা"});
    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 2);

    dictionary.remove_user_word("বাংলা");
    EXPECT_TRUE(dictionary.find_matches("bang").empty());
    EXPECT_TRUE(dictionary.find_matches("bngla").empty());
    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});
    EXPECT_TRUE(dictionary.list_user_words().empty());

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, LearnedSystemWordsAppearInPersonalListAndCanBeRemoved) {
    const std::string db_path =
        temporary_database_path("suborno_learned_system_word_lifecycle_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 10);
    dictionary.record_commit("বাংলা", "bangla");

    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 1);
    EXPECT_EQ(dictionary.list_user_words(), std::vector<std::string>{"বাংলা"});

    dictionary.remove_user_word("বাংলা");

    EXPECT_EQ(dictionary.get_user_frequency("বাংলা"), 0);
    EXPECT_TRUE(dictionary.list_user_words().empty());
    EXPECT_EQ(dictionary.find_matches("bangla"), std::vector<std::string>{"বাংলা"});

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, LearnedFrequencyChangesDictionaryRanking) {
    const std::string db_path =
        temporary_database_path("suborno_personal_learned_rank_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("আগেরশব্দ", "shared", 5);
    dictionary.add_entry("শেখাশব্দ", "shared", 1);

    ASSERT_EQ(dictionary.find_matches("shared").front(), "আগেরশব্দ");
    for (int count = 0; count < 5; ++count) {
        dictionary.record_commit("শেখাশব্দ", "shared");
    }

    const auto matches = dictionary.find_matches("shared");
    ASSERT_EQ(matches.size(), 2U);
    EXPECT_EQ(matches.front(), "শেখাশব্দ");
    EXPECT_EQ(dictionary.get_user_frequency("শেখাশব্দ"), 5);

    std::remove(db_path.c_str());
}

TEST(PersonalDictionaryTest, ImportsBundledBengaliWordlistExactlyOnce) {
    const std::filesystem::path dictionary_assets =
        std::filesystem::path(SUBORNO_SOURCE_DIR) / "assets" / "dictionaries";
    const std::vector<std::filesystem::path> word_lists{
        dictionary_assets / "bn-wordlist-unlicense-v1-part-01.txt",
        dictionary_assets / "bn-wordlist-unlicense-v1-part-02.txt",
        dictionary_assets / "bn-wordlist-unlicense-v1-part-03.txt",
        dictionary_assets / "bn-wordlist-unlicense-v1-part-04.txt"};
    for (const auto& word_list : word_lists) {
        ASSERT_TRUE(std::filesystem::is_regular_file(word_list)) << word_list.string();
    }

    const std::string db_path =
        temporary_database_path("suborno_bundled_bengali_wordlist_import_test.sqlite");
    std::remove(db_path.c_str());

    {
        suborno::dictionary::Dictionary dictionary(db_path);
        const auto start = std::chrono::steady_clock::now();
        EXPECT_EQ(dictionary.import_word_lists_if_needed(word_lists, "sanjidh090-bn-wordlist-d3095ea-v1"), 454649U);
        const auto elapsed = std::chrono::steady_clock::now() - start;
        RecordProperty(
            "bundled_wordlist_import_ms",
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());

        EXPECT_EQ(dictionary.find_matches("বাংলা"), std::vector<std::string>{"বাংলা"});
        EXPECT_EQ(dictionary.find_matches("বাংলাদেশ"), std::vector<std::string>{"বাংলাদেশ"});
        EXPECT_EQ(dictionary.import_word_lists_if_needed(word_lists, "sanjidh090-bn-wordlist-d3095ea-v1"), 0U);

        dictionary.add_user_word("নিজস্ব", "nijoshsho");
        dictionary.clear_user_entries();
        EXPECT_TRUE(dictionary.list_user_words().empty());
        EXPECT_TRUE(dictionary.find_matches("nijoshsho").empty());
        EXPECT_EQ(dictionary.find_matches("বাংলা"), std::vector<std::string>{"বাংলা"});
    }

    std::remove(db_path.c_str());
}
