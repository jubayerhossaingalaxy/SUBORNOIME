#include "suborno/layouts/phonetic/borno_phonetic_layout.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

TEST(BornoPhoneticLayoutTest, ConvertsIndependentAndDependentVowels) {
    const suborno::layouts::BornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("a"), "আ");
    EXPECT_EQ(layout.transliterate("ka"), "কা");
    EXPECT_EQ(layout.transliterate("o"), "অ");
    EXPECT_EQ(layout.transliterate("ko"), "ক");
    EXPECT_EQ(layout.transliterate("O"), "ও");
    EXPECT_EQ(layout.transliterate("kO"), "কো");
    EXPECT_EQ(layout.transliterate("ee"), "ঈ");
    EXPECT_EQ(layout.transliterate("kee"), "কী");
    EXPECT_EQ(layout.transliterate("rri"), "ঋ");
    EXPECT_EQ(layout.transliterate("krri"), "কৃ");
}

TEST(BornoPhoneticLayoutTest, PreservesCaseSensitiveConsonantTokens) {
    const suborno::layouts::BornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("t"), "ত");
    EXPECT_EQ(layout.transliterate("T"), "ট");
    EXPECT_EQ(layout.transliterate("th"), "থ");
    EXPECT_EQ(layout.transliterate("Th"), "ঠ");
    EXPECT_EQ(layout.transliterate("d"), "দ");
    EXPECT_EQ(layout.transliterate("D"), "ড");
    EXPECT_EQ(layout.transliterate("dh"), "ধ");
    EXPECT_EQ(layout.transliterate("Dh"), "ঢ");
    EXPECT_EQ(layout.transliterate("ng"), "ং");
    EXPECT_EQ(layout.transliterate("Ng"), "ঙ");
    EXPECT_EQ(layout.transliterate("NG"), "ঞ");
}

TEST(BornoPhoneticLayoutTest, ComposesConsonantRunsAndExplicitControls) {
    const suborno::layouts::BornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("kt"), "ক্‌ত");
    EXPECT_EQ(layout.transliterate("k,,t"), "ক্‌ত");
    EXPECT_EQ(layout.transliterate("k,.t"), "ক্‌\u200Cত");
    EXPECT_EQ(layout.transliterate("k,..t"), "ক্‌\u200Dত");
    EXPECT_EQ(layout.transliterate("D./"), "ড়");
    EXPECT_EQ(layout.transliterate("0123456789"), "০১২৩৪৫৬৭৮৯");
}

TEST(BornoPhoneticLayoutTest, EscapesLiteralGraphemesAndPreservesUnknownText) {
    const suborno::layouts::BornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate(R"(facebook\-এর)"), "facebook-এর");
    EXPECT_EQ(layout.transliterate("hello!"), "হেল্লো!");
    EXPECT_THROW(layout.transliterate("\xFF"), std::invalid_argument);
}

TEST(BornoPhoneticLayoutTest, ExposesIndependentCandidateIdentity) {
    const suborno::layouts::BornoPhoneticLayout layout;

    const auto candidates = layout.generate_candidates("ami");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "আমি");
    EXPECT_EQ(candidates.front().source, suborno::layout::CandidateSource::layout);
    EXPECT_EQ(candidates.front().stable_id, "rule-phonetic:আমি");
}
