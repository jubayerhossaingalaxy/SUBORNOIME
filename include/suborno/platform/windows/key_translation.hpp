#pragma once

#include <Windows.h>
#include <optional>

#include "suborno/layout-api/key_event.hpp"

namespace suborno::platform::windows {

[[nodiscard]] inline std::optional<layout::PhysicalKey> physical_key_from_virtual_key(
    WPARAM virtual_key) noexcept {
    if (virtual_key >= 'A' && virtual_key <= 'Z') {
        const auto key =
            static_cast<std::uint8_t>(static_cast<std::uint8_t>(layout::PhysicalKey::letter_a)
                                      + static_cast<std::uint8_t>(virtual_key - 'A'));
        return static_cast<layout::PhysicalKey>(key);
    }
    if (virtual_key >= '0' && virtual_key <= '9') {
        return static_cast<layout::PhysicalKey>(static_cast<std::uint8_t>(virtual_key));
    }
    if (virtual_key >= VK_NUMPAD0 && virtual_key <= VK_NUMPAD9) {
        const auto key =
            static_cast<std::uint8_t>(static_cast<std::uint8_t>(layout::PhysicalKey::numpad_0)
                                      + static_cast<std::uint8_t>(virtual_key - VK_NUMPAD0));
        return static_cast<layout::PhysicalKey>(key);
    }

    switch (virtual_key) {
    case VK_DECIMAL:
        return layout::PhysicalKey::numpad_decimal;
    case VK_OEM_1:
        return layout::PhysicalKey::semicolon;
    case VK_OEM_PLUS:
        return layout::PhysicalKey::equal;
    case VK_OEM_COMMA:
        return layout::PhysicalKey::comma;
    case VK_OEM_MINUS:
        return layout::PhysicalKey::minus;
    case VK_OEM_PERIOD:
        return layout::PhysicalKey::period;
    case VK_OEM_2:
        return layout::PhysicalKey::slash;
    case VK_OEM_3:
        return layout::PhysicalKey::grave;
    case VK_OEM_4:
        return layout::PhysicalKey::left_bracket;
    case VK_OEM_5:
    case VK_OEM_102:
        return layout::PhysicalKey::backslash;
    case VK_OEM_6:
        return layout::PhysicalKey::right_bracket;
    case VK_OEM_7:
        return layout::PhysicalKey::apostrophe;
    case VK_SPACE:
        return layout::PhysicalKey::space;
    default:
        return std::nullopt;
    }
}

}  // namespace suborno::platform::windows
