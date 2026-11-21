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

}  // namespace suborno::layout
