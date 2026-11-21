#include "suborno/layout-api/key_event.hpp"

#include <gtest/gtest.h>
#include <string>

namespace {

using suborno::layout::KeyEvent;
using suborno::layout::KeyModifiers;
using suborno::layout::PhysicalKey;
using suborno::layout::key_event_to_ascii;

TEST(KeyEventTest, AppliesShiftAndCapsLockAsAnXor) {
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::letter_a, {}}), 'a');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::letter_a, {.shift = true}}), 'A');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::letter_a, {.caps_lock = true}}), 'A');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::letter_a, {.shift = true, .caps_lock = true}}), 'a');
}

TEST(KeyEventTest, MapsShiftedNumberRowCharacters) {
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::digit_1, {}}), '1');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::digit_1, {.shift = true}}), '!');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::digit_0, {.shift = true}}), ')');
}

TEST(KeyEventTest, HonorsNumpadNumLockState) {
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::numpad_2, {.num_lock = true}}), '2');
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::numpad_2, {.num_lock = false}}).has_value());
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::numpad_decimal, {}}), '.');
}

TEST(KeyEventTest, MapsPunctuationAndShiftedVariants) {
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::semicolon, {}}), ';');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::semicolon, {.shift = true}}), ':');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::comma, {.shift = true}}), '<');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::slash, {.shift = true}}), '?');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::backslash, {}}), '\\');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::backslash, {.shift = true}}), '|');
}

TEST(KeyEventTest, RemapsNumpadDigitsToTheTopRowWhenNumLockIsEnabled) {
    KeyEvent numpad_two{
        .key = PhysicalKey::numpad_2,
        .modifiers = {.shift = true, .caps_lock = true, .num_lock = true},
        .scan_code = 0x50,
    };

    ASSERT_TRUE(suborno::layout::remap_numpad_to_top_row(numpad_two));
    EXPECT_EQ(numpad_two.key, PhysicalKey::digit_2);
    EXPECT_EQ(numpad_two.scan_code, 0x03U);
    EXPECT_FALSE(numpad_two.modifiers.shift);
    EXPECT_FALSE(numpad_two.modifiers.caps_lock);
    EXPECT_FALSE(numpad_two.logical_text_available);
    EXPECT_TRUE(numpad_two.logical_text_utf8.empty());
    EXPECT_EQ(key_event_to_ascii(numpad_two), '2');

    KeyEvent numpad_zero{
        .key = PhysicalKey::numpad_0,
        .modifiers = {.num_lock = true},
        .scan_code = 0x52,
    };
    ASSERT_TRUE(suborno::layout::remap_numpad_to_top_row(numpad_zero));
    EXPECT_EQ(numpad_zero.key, PhysicalKey::digit_0);
    EXPECT_EQ(numpad_zero.scan_code, 0x0BU);
}

TEST(KeyEventTest, DoesNotRemapNumpadWhenNumLockIsOff) {
    KeyEvent numpad_two{
        .key = PhysicalKey::numpad_2,
        .modifiers = {.num_lock = false},
        .scan_code = 0x50,
    };

    EXPECT_FALSE(suborno::layout::remap_numpad_to_top_row(numpad_two));
    EXPECT_EQ(numpad_two.key, PhysicalKey::numpad_2);
    EXPECT_EQ(numpad_two.scan_code, 0x50U);
}

TEST(KeyEventTest, IdentifiesNumericKeypadPhysicalKeys) {
    EXPECT_TRUE(suborno::layout::is_numpad_key(PhysicalKey::numpad_0));
    EXPECT_TRUE(suborno::layout::is_numpad_key(PhysicalKey::numpad_9));
    EXPECT_FALSE(suborno::layout::is_numpad_key(PhysicalKey::digit_0));
    EXPECT_FALSE(suborno::layout::is_numpad_key(PhysicalKey::letter_a));
}

TEST(KeyEventTest, DoesNotTreatModifierShortcutsAsText) {
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.control = true}}).has_value());
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.alt = true}}).has_value());
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.windows = true}}).has_value());
}

TEST(KeyEventTest, RecognizesAltGrAsPrintableInsteadOfAControlShortcut) {
    EXPECT_EQ(key_event_to_ascii(
                  {PhysicalKey::letter_a, {.control = true, .alt = true, .alt_graph = true}}),
              'a');
}

}  // namespace

TEST(KeyEventTest, DecodesWindowsRepeatScanCodeAndExtendedKeyBits) {
    const auto decoded = suborno::layout::decode_key_message_lparam(0xC0340003U);

    EXPECT_EQ(decoded.repeat_count, 3U);
    EXPECT_EQ(decoded.scan_code, 0x34U);
    EXPECT_TRUE(decoded.extended_key);
    EXPECT_TRUE(decoded.previous_key_state);
    EXPECT_FALSE(decoded.key_down);
}

TEST(KeyEventTest, DecodesFirstKeyDownMessage) {
    const auto decoded = suborno::layout::decode_key_message_lparam(0x001E0001U);

    EXPECT_EQ(decoded.repeat_count, 1U);
    EXPECT_EQ(decoded.scan_code, 0x1EU);
    EXPECT_FALSE(decoded.extended_key);
    EXPECT_FALSE(decoded.previous_key_state);
    EXPECT_TRUE(decoded.key_down);
}

TEST(KeyEventTest, DistinguishesUnavailableLogicalTextFromAnEmptyTranslation) {
    suborno::layout::KeyEvent event{
        .key = PhysicalKey::letter_a,
        .logical_text_available = true,
    };
    EXPECT_TRUE(event.logical_text_available);
    EXPECT_TRUE(event.logical_text_utf8.empty());
}
