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
}

TEST(KeyEventTest, MapsPunctuationAndShiftedVariants) {
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::semicolon, {}}), ';');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::semicolon, {.shift = true}}), ':');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::comma, {.shift = true}}), '<');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::slash, {.shift = true}}), '?');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::backslash, {}}), '\\');
    EXPECT_EQ(key_event_to_ascii({PhysicalKey::backslash, {.shift = true}}), '|');
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
