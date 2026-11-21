#pragma once

#include <span>
#include <string>
#include <string_view>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layouts::phonetic {

enum class PhoneticTokenKind {
    consonant,
    vowel,
    sign,
};

struct PhoneticTokenDefinition {
    std::string_view input;
    PhoneticTokenKind kind;
    std::string_view output;
    std::string_view dependent_output;
    bool nukta_eligible = true;
};

// A small immutable token table drives a complete, independent phonetic parser.
// The table must have static lifetime and contain unique, non-empty input tokens.
class RuleBasedPhoneticLayout : public layout::StatelessPhoneticLayout {
  public:
    explicit RuleBasedPhoneticLayout(std::span<const PhoneticTokenDefinition> tokens);

    std::string transliterate(const std::string& input) const override;
    std::vector<layout::Candidate> generate_candidates(const std::string& input) const override;

  private:
    std::span<const PhoneticTokenDefinition> tokens_;
};

}  // namespace suborno::layouts::phonetic
