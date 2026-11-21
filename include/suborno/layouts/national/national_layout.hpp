#pragma once

#include <string>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layouts {

class NationalLayout final : public layout::StatelessPhoneticLayout {
  public:
    std::string transliterate(const std::string& input) const override;
    std::vector<layout::Candidate> generate_candidates(const std::string& input) const override;
};

}  // namespace suborno::layouts
