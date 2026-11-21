#include "suborno/layouts/probhat/probhat_layout.hpp"

#include <gtest/gtest.h>

namespace {

suborno::layout::KeyEvent event(std::uint16_t scan_code,
                                bool shift = false,
                                bool caps_lock = false,
                                bool alt_graph = false) {
    return {
        .modifiers = {
            .shift = shift,
            .caps_lock = caps_lock,
            .control = alt_graph,
            .alt = alt_graph,
            .alt_graph = alt_graph,
        },
        .scan_code = scan_code,
        .key_down = true,
    };
}

}  // namespace

TEST(ProbhatLayoutTest, IsARealFixedPhysicalKeyboardLayout) {
    const suborno::layouts::ProbhatLayout layout;
    EXPECT_EQ(layout.kind(), suborno::layout::LayoutKind::fixed);
}

TEST(ProbhatLayoutTest, MapsPhysicalKeyPositionsAndShiftLayer) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("k"), "ক");
    EXPECT_EQ(layout.transliterate("K"), "খ");
    EXPECT_EQ(layout.transliterate("f"), "ত");
    EXPECT_EQ(layout.transliterate("F"), "থ");
    EXPECT_EQ(layout.transliterate("a"), "া");
    EXPECT_EQ(layout.transliterate("A"), "অ");
    EXPECT_EQ(layout.transliterate("q"), "দ");
    EXPECT_EQ(layout.transliterate("Q"), "ধ");
    EXPECT_EQ(layout.transliterate("z"), "য়");
    EXPECT_EQ(layout.transliterate("Z"), "য");
    EXPECT_EQ(layout.transliterate("y"), "এ");
    EXPECT_EQ(layout.transliterate("Y"), "ঐ");
    EXPECT_EQ(layout.transliterate("c"), "চ");
    EXPECT_EQ(layout.transliterate("C"), "ছ");
    EXPECT_EQ(layout.transliterate("w"), "ূ");
    EXPECT_EQ(layout.transliterate("W"), "ঊ");
    EXPECT_EQ(layout.transliterate("u"), "ু");
    EXPECT_EQ(layout.transliterate("U"), "উ");
    EXPECT_EQ(layout.transliterate("e"), "ী");
    EXPECT_EQ(layout.transliterate("E"), "ঈ");
    EXPECT_EQ(layout.transliterate("["), "ে");
    EXPECT_EQ(layout.transliterate("{"), "ৈ");
    EXPECT_EQ(layout.transliterate("]"), "ো");
    EXPECT_EQ(layout.transliterate("}"), "ৌ");
}

TEST(ProbhatLayoutTest, ConcatenatesLiteralFixedKeyOutputsWithoutPhoneticRewriting) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("ka"), "কা");
    EXPECT_EQ(layout.transliterate("ki"), "কি");
    EXPECT_EQ(layout.transliterate("ke"), "কী");
    EXPECT_EQ(layout.transliterate("ko"), "কও");
    EXPECT_EQ(layout.transliterate("k["), "কে");
    EXPECT_EQ(layout.transliterate("k]"), "কো");
    EXPECT_EQ(layout.transliterate("wa"), "ূা");
}

TEST(ProbhatLayoutTest, AppliesCapsLockOnlyToKeysMarkedInTheKlc) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(event(0x25, false, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "খ");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x02, false, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "১");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x33, false, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, ",");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x33, true, false)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ৃ");
}

TEST(ProbhatLayoutTest, MapsAltGrAndShiftAltGrLayersFromTheKlc) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(event(0x02, false, false, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "¡");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x02, true, false, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "¹");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x10, false, false, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ä");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x10, true, false, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "Ä");
}

TEST(ProbhatLayoutTest, AppliesCapsLockAltGrFlagOnlyWithinTheAltGrLayers) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(event(0x33, false, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, ",");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x33, false, true, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "Ç");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x33, true, true, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ç");
}

TEST(ProbhatLayoutTest, AppliesBothCapsLockFlagsToTheAltGrShiftLayer) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(event(0x10, false, true, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "Ä");

    session->reset();
    EXPECT_EQ(session->append_key(event(0x10, true, true, true)),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "ä");
}

TEST(ProbhatLayoutTest, DoesNotConsumeUnmappedAltGrLayers) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(event(0x21, false, false, true)),
              suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
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

TEST(ProbhatLayoutTest, AppliesShiftLayerFromPhysicalKeyEvents) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});

    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->append_key(event(0x25, true)), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().preedit_utf8, "খ");
}

TEST(ProbhatLayoutTest, PassesThroughUnmappedNumpadDigits) {
    const suborno::layouts::ProbhatLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(event(0x50)), suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().preedit_utf8.empty());
}

TEST(ProbhatLayoutTest, MapsDigitsSignsAndPunctuation) {
    const suborno::layouts::ProbhatLayout layout;

    EXPECT_EQ(layout.transliterate("1234567890"), "১২৩৪৫৬৭৮৯০");
    EXPECT_EQ(layout.transliterate("4"), "৪");
    EXPECT_EQ(layout.transliterate("!"), "!");
    EXPECT_EQ(layout.transliterate("@"), "@");
    EXPECT_EQ(layout.transliterate("#"), "#");
    EXPECT_EQ(layout.transliterate("$"), "৳");
    EXPECT_EQ(layout.transliterate("%"), "%");
    EXPECT_EQ(layout.transliterate("^"), "^");
    EXPECT_EQ(layout.transliterate("&"), "ঞ");
    EXPECT_EQ(layout.transliterate("*"), "ৎ");
    EXPECT_EQ(layout.transliterate("("), "(");
    EXPECT_EQ(layout.transliterate(")"), ")");
    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate(">"), "ঁ");
    EXPECT_EQ(layout.transliterate("/"), "্");
    EXPECT_EQ(layout.transliterate(","), ",");
    EXPECT_EQ(layout.transliterate("<"), "ৃ");
    EXPECT_EQ(layout.transliterate("\\"), "\\");
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
