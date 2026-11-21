#pragma once

#include <memory>
#include <string>
#include <vector>

#include "suborno/layout-api/input_layout.hpp"

namespace suborno::layouts {

class KhiproLayout final : public layout::InputLayout {
  public:
    layout::LayoutKind kind() const noexcept override {
        return layout::LayoutKind::compositional;
    }

    std::string transliterate(const std::string& input) const override;
    std::vector<layout::Candidate> generate_candidates(const std::string& input) const override;
    std::unique_ptr<layout::InputSession> create_session(
        dictionary::Dictionary* dictionary, layout::InputSessionOptions options) const override;
};

}  // namespace suborno::layouts
