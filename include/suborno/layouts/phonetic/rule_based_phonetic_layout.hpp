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
    halant_prefix,
};

struct PhoneticTokenDefinition {
    std::string_view input;
    PhoneticTokenKind kind;
    std::string_view output;
    // For vowels this is the dependent sign; for consonants it is the form
    // used after an existing consonant run (for example, ya-phala).
    std::string_view dependent_output;
    bool nukta_eligible = true;
};

// A small immutable token table drives a complete, independent phonetic parser.
// The table must have static lifetime and contain unique, non-empty input tokens.
class RuleBasedPhoneticLayout : public layout::StatelessPhoneticLayout {
  public:
    explicit RuleBasedPhoneticLayout(std::span<const PhoneticTokenDefinition> tokens,
                                     bool accent_key_enabled = false,
                                     std::string_view profile_id = "rule-phonetic");

    std::string transliterate(const std::string& input) const override;
    std::vector<layout::Candidate> generate_candidates(const std::string& input) const override;

  private:
    std::span<const PhoneticTokenDefinition> tokens_;
    bool accent_key_enabled_ = false;
    std::string_view profile_id_;
};

}  // namespace suborno::layouts::phonetic
