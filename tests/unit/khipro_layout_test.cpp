#include "suborno/layouts/khipro/khipro_layout.hpp"

#include <gtest/gtest.h>

TEST(KhiproLayoutTest, BasicConsonantAndVowelComposition) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.kind(), suborno::layout::LayoutKind::compositional);
    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    EXPECT_EQ(layout.transliterate("kaka"), "কাকা");
    EXPECT_EQ(layout.transliterate("bangla"), "বাংলা");
    EXPECT_EQ(layout.transliterate("tfaka"), "টাকা");
    EXPECT_EQ(layout.transliterate("dffaka"), "ঢাকা");
}

TEST(KhiproLayoutTest, AutomaticallyJoinsAdjacentConsonants) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate("kt"), "ক্ত");
    EXPECT_EQ(layout.transliterate("k;t"), "কত");
    EXPECT_EQ(layout.transliterate("kor"), "কর");
}

TEST(KhiproLayoutTest, SupportsForcedVowelForms) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate("amii"), "আমী");
    EXPECT_EQ(layout.transliterate("amiif"), "আমিই");
    EXPECT_EQ(layout.transliterate("valwi"), "ভালৈ");
    EXPECT_EQ(layout.transliterate("valwif"), "ভালোই");
    EXPECT_EQ(layout.transliterate("nwuka"), "নৌকা");
    EXPECT_EQ(layout.transliterate("nwufka"), "নোউকা");
}

TEST(KhiproLayoutTest, SupportsSlicerAndChandrabindu) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate("ta/der"), "তাঁদের");
    EXPECT_EQ(layout.transliterate("bakr/uddh"), "বাকরুদ্ধ");
}

TEST(KhiproLayoutTest, SupportsSpecialCharactersAndLiteralSeparator) {
    const suborno::layouts::KhiproLayout layout;

    EXPECT_EQ(layout.transliterate(";;"), ";");
    EXPECT_EQ(layout.transliterate("x"), "ং");
    EXPECT_EQ(layout.transliterate("oo"), "ঃ");
    EXPECT_EQ(layout.transliterate("t/"), "ৎ");
    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate(".f"), "॥");
    EXPECT_EQ(layout.transliterate("$"), "৳");
    EXPECT_EQ(layout.transliterate("$f"), "₹");
}

TEST(KhiproLayoutTest, CreatesCompositionSessionsAndCommitsTheSelectedOutput) {
    const suborno::layouts::KhiproLayout layout;
    auto session = layout.create_session(nullptr, {.candidate_limit = 4});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_utf8("kt"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক্ত");
    ASSERT_FALSE(session->snapshot().candidates.empty());
    EXPECT_EQ(session->commit().text_utf8, "ক্ত");
}
