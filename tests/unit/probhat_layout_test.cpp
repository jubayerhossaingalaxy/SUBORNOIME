#include "suborno/layouts/probhat/probhat_layout.hpp"

#include <gtest/gtest.h>

namespace {

suborno::layout::KeyEvent event(std::uint8_t scan_code, bool shift = false) {
    return {
        .modifiers = {.shift = shift},
        .scan_code = scan_code,
        .key_down = true,
    };
}

}  // namespace

TEST(ProbhatLayoutTest, IsARealFixedPhysicalKeyboardLayout) {
    const suborno::layouts::ProbhatLayout layout;
    EXPECT_EQ(layout.kind(), suborno::layout::LayoutKind::fixed);
}

TEST(ProbhatLayoutTest, MapsPhysicalLetterKeysAndShiftLayer) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("k"), "ক");
    EXPECT_EQ(layout.transliterate("K"), "খ");
    EXPECT_EQ(layout.transliterate("f"), "ত");
    EXPECT_EQ(layout.transliterate("F"), "থ");
    EXPECT_EQ(layout.transliterate("a"), "আ");
    EXPECT_EQ(layout.transliterate("A"), "অ");
    EXPECT_EQ(layout.transliterate("q"), "দ");
    EXPECT_EQ(layout.transliterate("Q"), "ধ");
    EXPECT_EQ(layout.transliterate("z"), "য়");
    EXPECT_EQ(layout.transliterate("Z"), "য");
    EXPECT_EQ(layout.transliterate("y"), "এ");
    EXPECT_EQ(layout.transliterate("Y"), "ঐ");
    EXPECT_EQ(layout.transliterate("ka"), "কা");
    EXPECT_EQ(layout.transliterate("ki"), "কি");
    EXPECT_EQ(layout.transliterate("ke"), "কে");
    EXPECT_EQ(layout.transliterate("ko"), "কো");
}

TEST(ProbhatLayoutTest, MapsDigitsSignsAndPunctuation) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("1234567890"), "১২৩৪৫৬৭৮৯০");
    EXPECT_EQ(layout.transliterate("4"), "৪");
    EXPECT_EQ(layout.transliterate("7"), "৭");
    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate("/"), "্");
    EXPECT_EQ(layout.transliterate(","), ",");
}

TEST(ProbhatLayoutTest, UsesPhysicalScanCodeInsteadOfTheActiveLogicalCharacter) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    const suborno::layout::KeyEvent physical_k{
        .key = suborno::layout::PhysicalKey::letter_a,
        .scan_code = 0x25,
        .logical_text_available = true,
        .logical_text_utf8 = "য",
    };
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(physical_k), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
}

TEST(ProbhatLayoutTest, AppliesShiftLayerFromThePhysicalKeyEvent) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(event(0x25, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "খ");
}

TEST(ProbhatLayoutTest, AppendsDependentVowelsAcrossSessionKeyEvents) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_utf8("k"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
    EXPECT_EQ(session->append_utf8("a"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কা");
    EXPECT_EQ(session->append_utf8("i"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কাই");
}

TEST(ProbhatLayoutTest, MapsNumpadDigitsWhenNumLockIsEnabled) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_NE(session, nullptr);
    suborno::layout::KeyEvent numpad_event{
        .key = suborno::layout::PhysicalKey::numpad_2,
        .modifiers = {.num_lock = true},
    };
    EXPECT_EQ(session->append_key(numpad_event), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "২");
}

TEST(ProbhatLayoutTest, BackspaceRemovesOneBengaliGrapheme) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(event(0x25)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(event(0x21)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "কত");
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
    EXPECT_EQ(session->commit().text_utf8, "ক");
}
