#include "suborno/layout-api/key_event.hpp"

#include <gtest/gtest.h>

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

TEST(KeyEventTest, DoesNotTreatModifierShortcutsAsText) {
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.control = true}}).has_value());
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.alt = true}}).has_value());
    EXPECT_FALSE(key_event_to_ascii({PhysicalKey::letter_a, {.windows = true}}).has_value());
}

}  // namespace
