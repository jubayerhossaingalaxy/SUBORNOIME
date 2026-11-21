#include <filesystem>
#include <gtest/gtest.h>
#include <string>

#include "suborno/dictionary/dictionary.hpp"

namespace {

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

}  // namespace

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
