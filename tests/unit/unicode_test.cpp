#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "suborno/unicode/bengali.hpp"

TEST(UnicodeUtilitiesTest, ValidatesUtf8WithoutAcceptingMalformedSequences) {
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("বাংলা"));
    EXPECT_TRUE(suborno::unicode::is_valid_utf8("ASCII"));
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xC0\xAF"));
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xE2\x82"));
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xED\xA0\x80"));
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\xF4\x90\x80\x80"));
    EXPECT_FALSE(suborno::unicode::is_valid_utf8("\x80"));
}

TEST(UnicodeUtilitiesTest, CountsUnicodeCodePointsRatherThanUtf8Bytes) {
    EXPECT_EQ(suborno::unicode::count_utf8_code_points("Aকি"), 3U);
    EXPECT_EQ(suborno::unicode::count_utf8_code_points(""), 0U);
    EXPECT_THROW(suborno::unicode::count_utf8_code_points("\xC0"), std::invalid_argument);
}

TEST(UnicodeUtilitiesTest, NormalizesNfcAndNfkcExplicitly) {
    EXPECT_EQ(suborno::unicode::normalize_nfc("\u09DC"), "\u09A1\u09BC");
    EXPECT_EQ(suborno::unicode::normalize_nfc("\u09DD"), "\u09A2\u09BC");
    EXPECT_EQ(suborno::unicode::normalize_nfc("\u09DF"), "\u09AF\u09BC");
    EXPECT_EQ(suborno::unicode::normalize_nfc("\u09A1\u09BC"), "\u09A1\u09BC");
    EXPECT_EQ(suborno::unicode::normalize_nfkc("\uFF21"), "A");
    EXPECT_THROW(suborno::unicode::normalize_nfc("\xF0\x80"), std::invalid_argument);
}

TEST(UnicodeUtilitiesTest, SplitsExtendedGraphemeClusters) {
    const auto graphemes = suborno::unicode::split_graphemes("কাx");

    ASSERT_EQ(graphemes.size(), 2U);
    EXPECT_EQ(graphemes[0], "কা");
    EXPECT_EQ(graphemes[1], "x");
    EXPECT_THROW(suborno::unicode::split_graphemes("\xE2\x82"), std::invalid_argument);
}

TEST(UnicodeUtilitiesTest, SplitsEmojiRegionalIndicatorsAndCombiningSequences) {
    const auto emoji = suborno::unicode::split_graphemes("\U0001F469\u200D\U0001F4BBx");
    ASSERT_EQ(emoji.size(), 2U);
    EXPECT_EQ(emoji[0], "\U0001F469\u200D\U0001F4BB");
    EXPECT_EQ(emoji[1], "x");

    const auto flag = suborno::unicode::split_graphemes("\U0001F1E7\U0001F1E9x");
    ASSERT_EQ(flag.size(), 2U);
    EXPECT_EQ(flag[0], "\U0001F1E7\U0001F1E9");
    EXPECT_EQ(flag[1], "x");

    const auto combining = suborno::unicode::split_graphemes("e\u0301x");
    ASSERT_EQ(combining.size(), 2U);
    EXPECT_EQ(combining[0], "e\u0301");
    EXPECT_EQ(combining[1], "x");
}
