#include "suborno/spell/spell_checker.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>

#include "suborno/dictionary/dictionary.hpp"

namespace {

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

}  // namespace

TEST(SpellCheckerTest, CorrectWordIsAccepted) {
    const std::string db_path = temporary_database_path("suborno_spell_correct_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 5);

    suborno::spell::SpellChecker checker(dictionary);
    const auto result = checker.check("bangla");

    EXPECT_TRUE(result.is_correct);
    EXPECT_TRUE(result.suggestions.empty());

    std::remove(db_path.c_str());
}

TEST(SpellCheckerTest, UnknownWordGetsSuggestions) {
    const std::string db_path = temporary_database_path("suborno_spell_suggest_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "bangla", 5);
    dictionary.add_entry("ভারত", "bharat", 3);

    suborno::spell::SpellChecker checker(dictionary);
    const auto result = checker.check("banglaa");

    EXPECT_FALSE(result.is_correct);
    // Should have some suggestions
    EXPECT_FALSE(result.suggestions.empty());

    std::remove(db_path.c_str());
}

TEST(SpellCheckerTest, EditDistanceIsCorrect) {
    const std::string db_path = temporary_database_path("suborno_spell_edit_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    suborno::spell::SpellChecker checker(dictionary);

    EXPECT_DOUBLE_EQ(checker.edit_distance("", ""), 0.0);
    EXPECT_DOUBLE_EQ(checker.edit_distance("a", ""), 1.0);
    EXPECT_DOUBLE_EQ(checker.edit_distance("", "a"), 1.0);
    EXPECT_DOUBLE_EQ(checker.edit_distance("a", "a"), 0.0);
    EXPECT_DOUBLE_EQ(checker.edit_distance("a", "b"), 1.0);
    EXPECT_DOUBLE_EQ(checker.edit_distance("ab", "ba"), 2.0);
    EXPECT_DOUBLE_EQ(checker.edit_distance("kitten", "sitting"), 3.0);

    std::remove(db_path.c_str());
}

TEST(SpellCheckerTest, GenerateEditsProducesVariants) {
    const std::string db_path = temporary_database_path("suborno_spell_edits_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    suborno::spell::SpellChecker checker(dictionary);

    const auto edits = checker.generate_edits("ab");
    EXPECT_FALSE(edits.empty());

    // Deletion of 'a' should produce "b"
    bool has_b = false;
    for (const auto& e : edits) {
        if (e == "b")
            has_b = true;
    }
    EXPECT_TRUE(has_b);

    std::remove(db_path.c_str());
}
