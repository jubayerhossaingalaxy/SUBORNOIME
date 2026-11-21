#include "suborno/layouts/national/national_layout.hpp"

#include <gtest/gtest.h>

TEST(NationalLayoutTest, BasicConsonantTransliteration) {
    suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("k"), "ক");
    EXPECT_EQ(layout.transliterate("kh"), "খ");
    EXPECT_EQ(layout.transliterate("g"), "গ");
    EXPECT_EQ(layout.transliterate("gh"), "ঘ");
    EXPECT_EQ(layout.transliterate("c"), "চ");
    EXPECT_EQ(layout.transliterate("ch"), "ছ");
    EXPECT_EQ(layout.transliterate("j"), "জ");
    EXPECT_EQ(layout.transliterate("jh"), "ঝ");
    EXPECT_EQ(layout.transliterate("T"), "ট");
    EXPECT_EQ(layout.transliterate("Th"), "ঠ");
    EXPECT_EQ(layout.transliterate("D"), "ড");
    EXPECT_EQ(layout.transliterate("Dh"), "ঢ");
    EXPECT_EQ(layout.transliterate("N"), "ণ");
    EXPECT_EQ(layout.transliterate("t"), "ত");
    EXPECT_EQ(layout.transliterate("th"), "থ");
    EXPECT_EQ(layout.transliterate("d"), "দ");
    EXPECT_EQ(layout.transliterate("dh"), "ধ");
    EXPECT_EQ(layout.transliterate("n"), "ন");
    EXPECT_EQ(layout.transliterate("p"), "প");
    EXPECT_EQ(layout.transliterate("ph"), "ফ");
    EXPECT_EQ(layout.transliterate("b"), "ব");
    EXPECT_EQ(layout.transliterate("bh"), "ভ");
    EXPECT_EQ(layout.transliterate("m"), "ম");
    EXPECT_EQ(layout.transliterate("y"), "য");
    EXPECT_EQ(layout.transliterate("r"), "র");
    EXPECT_EQ(layout.transliterate("l"), "ল");
    EXPECT_EQ(layout.transliterate("sh"), "শ");
    EXPECT_EQ(layout.transliterate("s"), "স");
    EXPECT_EQ(layout.transliterate("h"), "হ");
}

TEST(NationalLayoutTest, VowelTransliteration) {
    suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("a"), "আ");
    EXPECT_EQ(layout.transliterate("A"), "অ");
    EXPECT_EQ(layout.transliterate("i"), "ই");
    EXPECT_EQ(layout.transliterate("I"), "ঈ");
    EXPECT_EQ(layout.transliterate("u"), "উ");
    EXPECT_EQ(layout.transliterate("U"), "ঊ");
    EXPECT_EQ(layout.transliterate("e"), "এ");
    EXPECT_EQ(layout.transliterate("ai"), "ঐ");
    EXPECT_EQ(layout.transliterate("o"), "ও");
    EXPECT_EQ(layout.transliterate("au"), "ঔ");
}

TEST(NationalLayoutTest, Signs) {
    suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("M"), "ং");
    EXPECT_EQ(layout.transliterate("H"), "ঃ");
    EXPECT_EQ(layout.transliterate("kM"), "কং");
    EXPECT_EQ(layout.transliterate("kH"), "কঃ");
}

TEST(NationalLayoutTest, Clusters) {
    suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("kSh"), "ক্ষ");
    EXPECT_EQ(layout.transliterate("GY"), "জ্ঞ");
    EXPECT_EQ(layout.transliterate("JN"), "জ্ঞ");
}

TEST(NationalLayoutTest, Words) {
    suborno::layouts::NationalLayout layout;

    EXPECT_EQ(layout.transliterate("ami"), "\u0986\u09AE\u09BF");
    EXPECT_EQ(layout.transliterate("bangla"), "\u09AC\u09BE\u0982\u0997\u09B2\u09BE");
}

TEST(NationalLayoutTest, Candidates) {
    suborno::layouts::NationalLayout layout;

    const auto candidates = layout.generate_candidates("ami");
    ASSERT_EQ(candidates.size(), 1U);
    EXPECT_EQ(candidates.front().text, "আমি");
    EXPECT_EQ(candidates.front().source, suborno::layout::CandidateSource::layout);
}
