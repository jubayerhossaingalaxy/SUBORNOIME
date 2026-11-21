#pragma once

#include <memory>

#include "suborno/layout-api/input_layout.hpp"

namespace suborno::layout {

class PhoneticLayout : public InputLayout {
  public:
    LayoutKind kind() const noexcept final {
        return LayoutKind::phonetic;
    }

    std::unique_ptr<InputSession> create_session(
        dictionary::Dictionary* dictionary,
        InputSessionOptions options) const override;
};

// Use this base when a phonetic layout has its own parser and must not reuse
// the Suborno-specific parser state machine.
class StatelessPhoneticLayout : public PhoneticLayout {
  public:
    std::unique_ptr<InputSession> create_session(
        dictionary::Dictionary* dictionary,
        InputSessionOptions options) const override;
};

}  // namespace suborno::layout
