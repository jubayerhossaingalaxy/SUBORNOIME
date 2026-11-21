#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace suborno::layout {

enum class PhysicalKey : std::uint8_t {
    unknown = 0,
    letter_a = 'a',
    letter_b = 'b',
    letter_c = 'c',
    letter_d = 'd',
    letter_e = 'e',
    letter_f = 'f',
    letter_g = 'g',
    letter_h = 'h',
    letter_i = 'i',
    letter_j = 'j',
    letter_k = 'k',
    letter_l = 'l',
    letter_m = 'm',
    letter_n = 'n',
    letter_o = 'o',
    letter_p = 'p',
    letter_q = 'q',
    letter_r = 'r',
    letter_s = 's',
    letter_t = 't',
    letter_u = 'u',
    letter_v = 'v',
    letter_w = 'w',
    letter_x = 'x',
    letter_y = 'y',
    letter_z = 'z',
    digit_0 = '0',
    digit_1 = '1',
    digit_2 = '2',
    digit_3 = '3',
    digit_4 = '4',
    digit_5 = '5',
    digit_6 = '6',
    digit_7 = '7',
    digit_8 = '8',
    digit_9 = '9',
    numpad_0 = 128,
    numpad_1,
    numpad_2,
    numpad_3,
    numpad_4,
    numpad_5,
    numpad_6,
    numpad_7,
    numpad_8,
    numpad_9,
    semicolon = 160,
    equal,
    comma,
    minus,
    period,
    slash,
    grave,
    left_bracket,
    backslash,
    right_bracket,
    apostrophe,
    space,
};

struct KeyModifiers {
    bool shift = false;
    bool caps_lock = false;
    bool num_lock = false;
    bool control = false;
    bool alt = false;
    bool windows = false;
};

struct KeyEvent {
    PhysicalKey key = PhysicalKey::unknown;
    KeyModifiers modifiers;
};

[[nodiscard]] constexpr std::optional<char> key_event_to_ascii(
    const KeyEvent& event) noexcept {
    if (event.modifiers.control || event.modifiers.alt || event.modifiers.windows) {
        return std::nullopt;
    }

    const auto value = static_cast<std::uint8_t>(event.key);
    const auto first_letter = static_cast<std::uint8_t>(PhysicalKey::letter_a);
    const auto last_letter = static_cast<std::uint8_t>(PhysicalKey::letter_z);
    if (value >= first_letter && value <= last_letter) {
        const char lower = static_cast<char>(value);
        const bool uppercase = event.modifiers.shift != event.modifiers.caps_lock;
        return uppercase ? static_cast<char>(lower - ('a' - 'A')) : lower;
    }

    const auto first_digit = static_cast<std::uint8_t>(PhysicalKey::digit_0);
    const auto last_digit = static_cast<std::uint8_t>(PhysicalKey::digit_9);
    if (value >= first_digit && value <= last_digit) {
        constexpr char kUnshifted[] = "0123456789";
        constexpr char kShifted[] = ")!@#$%^&*(";
        const std::size_t index = static_cast<std::size_t>(value - first_digit);
        return event.modifiers.shift ? kShifted[index] : kUnshifted[index];
    }

    const auto first_numpad = static_cast<std::uint8_t>(PhysicalKey::numpad_0);
    const auto last_numpad = static_cast<std::uint8_t>(PhysicalKey::numpad_9);
    if (value >= first_numpad && value <= last_numpad) {
        if (!event.modifiers.num_lock) {
            return std::nullopt;
        }
        return static_cast<char>('0' + (value - first_numpad));
    }

    const bool shift = event.modifiers.shift;
    switch (event.key) {
    case PhysicalKey::semicolon:
        return shift ? ':' : ';';
    case PhysicalKey::equal:
        return shift ? '+' : '=';
    case PhysicalKey::comma:
        return shift ? '<' : ',';
    case PhysicalKey::minus:
        return shift ? '_' : '-';
    case PhysicalKey::period:
        return shift ? '>' : '.';
    case PhysicalKey::slash:
        return shift ? '?' : '/';
    case PhysicalKey::grave:
        return shift ? '~' : '`';
    case PhysicalKey::left_bracket:
        return shift ? '{' : '[';
    case PhysicalKey::backslash:
        return shift ? '|' : '\\';
    case PhysicalKey::right_bracket:
        return shift ? '}' : ']';
    case PhysicalKey::apostrophe:
        return shift ? '"' : '\'';
    case PhysicalKey::space:
        return ' ';
    default:
        return std::nullopt;
    }
}

}  // namespace suborno::layout
