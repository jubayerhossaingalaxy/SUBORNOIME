#include "suborno/layouts/phonetic/borno_phonetic_layout.hpp"

#include <array>

namespace suborno::layouts {
namespace {

using phonetic::PhoneticTokenDefinition;
using phonetic::PhoneticTokenKind;

constexpr std::array kBornoTokens{
    // Independent and dependent vowel forms.
    PhoneticTokenDefinition{"rri", PhoneticTokenKind::vowel, "ঋ", "ৃ"},
    PhoneticTokenDefinition{"ee", PhoneticTokenKind::vowel, "ঈ", "ী"},
    PhoneticTokenDefinition{"OI", PhoneticTokenKind::vowel, "ঐ", "ৈ"},
    PhoneticTokenDefinition{"OU", PhoneticTokenKind::vowel, "ঔ", "ৌ"},
    PhoneticTokenDefinition{"oo", PhoneticTokenKind::vowel, "উ", "ু"},
    PhoneticTokenDefinition{"a", PhoneticTokenKind::vowel, "আ", "া"},
    PhoneticTokenDefinition{"i", PhoneticTokenKind::vowel, "ই", "ি"},
    PhoneticTokenDefinition{"I", PhoneticTokenKind::vowel, "ঈ", "ী"},
    PhoneticTokenDefinition{"u", PhoneticTokenKind::vowel, "উ", "ু"},
    PhoneticTokenDefinition{"U", PhoneticTokenKind::vowel, "ঊ", "ূ"},
    PhoneticTokenDefinition{"e", PhoneticTokenKind::vowel, "এ", "ে"},
    PhoneticTokenDefinition{"o", PhoneticTokenKind::vowel, "অ", ""},
    PhoneticTokenDefinition{"O", PhoneticTokenKind::vowel, "ও", "ো"},

    // Consonants. Capital tokens retain their explicit phonetic meaning.
    PhoneticTokenDefinition{"Ng", PhoneticTokenKind::consonant, "ঙ", {}, false},
    PhoneticTokenDefinition{"NG", PhoneticTokenKind::consonant, "ঞ", {}, false},
    PhoneticTokenDefinition{"kh", PhoneticTokenKind::consonant, "খ", {}, false},
    PhoneticTokenDefinition{"gh", PhoneticTokenKind::consonant, "ঘ", {}, false},
    PhoneticTokenDefinition{"ch", PhoneticTokenKind::consonant, "ছ", {}, false},
    PhoneticTokenDefinition{"jh", PhoneticTokenKind::consonant, "ঝ", {}, false},
    PhoneticTokenDefinition{"Th", PhoneticTokenKind::consonant, "ঠ", {}, false},
    PhoneticTokenDefinition{"Dh", PhoneticTokenKind::consonant, "ঢ", {}, false},
    PhoneticTokenDefinition{"Sh", PhoneticTokenKind::consonant, "ষ", {}, false},
    PhoneticTokenDefinition{"Rh", PhoneticTokenKind::consonant, "ঢ়", {}, false},
    PhoneticTokenDefinition{"ph", PhoneticTokenKind::consonant, "ফ", {}, false},
    PhoneticTokenDefinition{"bh", PhoneticTokenKind::consonant, "ভ", {}, false},
    PhoneticTokenDefinition{"th", PhoneticTokenKind::consonant, "থ", {}, false},
    PhoneticTokenDefinition{"dh", PhoneticTokenKind::consonant, "ধ", {}, false},
    PhoneticTokenDefinition{"ng", PhoneticTokenKind::sign, "ং", {}},
    PhoneticTokenDefinition{"k", PhoneticTokenKind::consonant, "ক", {}},
    PhoneticTokenDefinition{"g", PhoneticTokenKind::consonant, "গ", {}},
    PhoneticTokenDefinition{"c", PhoneticTokenKind::consonant, "চ", {}},
    PhoneticTokenDefinition{"j", PhoneticTokenKind::consonant, "জ", {}},
    PhoneticTokenDefinition{"T", PhoneticTokenKind::consonant, "ট", {}, false},
    PhoneticTokenDefinition{"D", PhoneticTokenKind::consonant, "ড", {}},
    PhoneticTokenDefinition{"N", PhoneticTokenKind::consonant, "ণ", {}, false},
    PhoneticTokenDefinition{"t", PhoneticTokenKind::consonant, "ত", {}},
    PhoneticTokenDefinition{"d", PhoneticTokenKind::consonant, "দ", {}},
    PhoneticTokenDefinition{"n", PhoneticTokenKind::consonant, "ন", {}},
    PhoneticTokenDefinition{"p", PhoneticTokenKind::consonant, "প", {}},
    PhoneticTokenDefinition{"f", PhoneticTokenKind::consonant, "ফ", {}},
    PhoneticTokenDefinition{"b", PhoneticTokenKind::consonant, "ব", {}},
    PhoneticTokenDefinition{"v", PhoneticTokenKind::consonant, "ভ", {}},
    PhoneticTokenDefinition{"m", PhoneticTokenKind::consonant, "ম", {}},
    PhoneticTokenDefinition{"z", PhoneticTokenKind::consonant, "য", {}},
    PhoneticTokenDefinition{"r", PhoneticTokenKind::consonant, "র", {}},
    PhoneticTokenDefinition{"l", PhoneticTokenKind::consonant, "ল", {}},
    PhoneticTokenDefinition{"sh", PhoneticTokenKind::consonant, "শ", {}},
    PhoneticTokenDefinition{"S", PhoneticTokenKind::consonant, "শ", {}},
    PhoneticTokenDefinition{"s", PhoneticTokenKind::consonant, "স", {}},
    PhoneticTokenDefinition{"h", PhoneticTokenKind::consonant, "হ", {}},
    PhoneticTokenDefinition{"R", PhoneticTokenKind::consonant, "ড়", {}, false},
    PhoneticTokenDefinition{"y", PhoneticTokenKind::consonant, "য়", {}, false},
    PhoneticTokenDefinition{"Y", PhoneticTokenKind::consonant, "য়", {}, false},
    PhoneticTokenDefinition{"X", PhoneticTokenKind::sign, "ৎ", {}},
    PhoneticTokenDefinition{"TH", PhoneticTokenKind::sign, "ৎ", {}},
    PhoneticTokenDefinition{"t\`\`", PhoneticTokenKind::sign, "ৎ", {}},
};

}  // namespace

BornoPhoneticLayout::BornoPhoneticLayout()
    : RuleBasedPhoneticLayout(kBornoTokens) {}

}  // namespace suborno::layouts
