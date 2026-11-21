#pragma once

#include "suborno/layouts/phonetic/rule_based_phonetic_layout.hpp"

namespace suborno::layouts {

class AvroPhoneticLayout final : public phonetic::RuleBasedPhoneticLayout {
  public:
    AvroPhoneticLayout();
};

}  // namespace suborno::layouts
