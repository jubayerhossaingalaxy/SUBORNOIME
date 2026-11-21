#pragma once

#include "suborno/layouts/phonetic/rule_based_phonetic_layout.hpp"

namespace suborno::layouts {

class BornoPhoneticLayout final : public phonetic::RuleBasedPhoneticLayout {
  public:
    BornoPhoneticLayout();
};

}  // namespace suborno::layouts
