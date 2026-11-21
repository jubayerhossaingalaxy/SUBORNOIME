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
