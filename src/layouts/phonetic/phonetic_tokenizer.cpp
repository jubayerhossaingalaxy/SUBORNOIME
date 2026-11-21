#include "phonetic_tokenizer.hpp"

#include <array>

namespace suborno::layouts::phonetic::detail {
namespace {

using Kind = TokenKind;
using Definition = TokenDefinition;

constexpr std::array kTokens{
    Definition{
        .input = "chh",
        .kind = Kind::consonant,
        .output = "ছ",
        .dependent_output = {},
    },
    Definition{.input = "ksh", .kind = Kind::cluster, .output = "ক্ষ", .dependent_output = {}},
    Definition{.input = "kkh", .kind = Kind::cluster, .output = "ক্ষ", .dependent_output = {}},
    Definition{.input = "jny", .kind = Kind::cluster, .output = "জ্ঞ", .dependent_output = {}},
    Definition{.input = "Th", .kind = Kind::consonant, .output = "ঠ", .dependent_output = {}},
    Definition{.input = "Dh", .kind = Kind::consonant, .output = "ঢ", .dependent_output = {}},
    Definition{.input = "Rh", .kind = Kind::consonant, .output = "ঢ়", .dependent_output = {}},
    Definition{.input = "Ch", .kind = Kind::consonant, .output = "ছ", .dependent_output = {}},
    Definition{.input = "Gh", .kind = Kind::consonant, .output = "ঘ", .dependent_output = {}},
    Definition{.input = "Jh", .kind = Kind::consonant, .output = "ঝ", .dependent_output = {}},
    Definition{.input = "Kh", .kind = Kind::consonant, .output = "খ", .dependent_output = {}},
    Definition{.input = "Ph", .kind = Kind::consonant, .output = "ফ", .dependent_output = {}},
    Definition{.input = "Bh", .kind = Kind::consonant, .output = "ভ", .dependent_output = {}},
    Definition{.input = "aa", .kind = Kind::vowel, .output = "আ", .dependent_output = "া"},
    Definition{.input = "ii", .kind = Kind::vowel, .output = "ঈ", .dependent_output = "ী"},
    Definition{.input = "ee", .kind = Kind::vowel, .output = "ঈ", .dependent_output = "ী"},
    Definition{.input = "uu", .kind = Kind::vowel, .output = "ঊ", .dependent_output = "ূ"},
    Definition{.input = "oo", .kind = Kind::vowel, .output = "ঊ", .dependent_output = "ূ"},
    Definition{.input = "ri", .kind = Kind::vowel, .output = "ঋ", .dependent_output = "ৃ"},
    Definition{.input = "oi", .kind = Kind::vowel, .output = "ঐ", .dependent_output = "ৈ"},
    Definition{.input = "ai", .kind = Kind::vowel, .output = "ঐ", .dependent_output = "ৈ"},
    Definition{.input = "ou", .kind = Kind::vowel, .output = "ঔ", .dependent_output = "ৌ"},
    Definition{.input = "au", .kind = Kind::vowel, .output = "ঔ", .dependent_output = "ৌ"},
    Definition{.input = "kh", .kind = Kind::consonant, .output = "খ", .dependent_output = {}},
    Definition{.input = "gh", .kind = Kind::consonant, .output = "ঘ", .dependent_output = {}},
    Definition{.input = "ch", .kind = Kind::consonant, .output = "ছ", .dependent_output = {}},
    Definition{.input = "jh", .kind = Kind::consonant, .output = "ঝ", .dependent_output = {}},
    Definition{.input = "ny", .kind = Kind::consonant, .output = "ঞ", .dependent_output = {}},
    Definition{.input = "th", .kind = Kind::consonant, .output = "থ", .dependent_output = {}},
    Definition{.input = "dh", .kind = Kind::consonant, .output = "ধ", .dependent_output = {}},
    Definition{.input = "ph", .kind = Kind::consonant, .output = "ফ", .dependent_output = {}},
    Definition{.input = "bh", .kind = Kind::consonant, .output = "ভ", .dependent_output = {}},
    Definition{.input = "ng", .kind = Kind::nasal_ng, .output = "ং", .dependent_output = "ঙ"},
    Definition{.input = "gy", .kind = Kind::cluster, .output = "জ্ঞ", .dependent_output = {}},
    Definition{.input = "jn", .kind = Kind::cluster, .output = "জ্ঞ", .dependent_output = {}},
    Definition{
        .input = "rh",
        .kind = Kind::consonant,
        .output = "র\u09CDহ",
        .dependent_output = {},
    },
    Definition{.input = "sh", .kind = Kind::consonant, .output = "শ", .dependent_output = {}},
    Definition{.input = "Sh", .kind = Kind::consonant, .output = "ষ", .dependent_output = {}},
    Definition{.input = "a", .kind = Kind::vowel, .output = "আ", .dependent_output = "া"},
    Definition{.input = "ô", .kind = Kind::vowel, .output = "অ", .dependent_output = ""},
    Definition{.input = "O", .kind = Kind::vowel, .output = "অ", .dependent_output = ""},
    Definition{.input = "i", .kind = Kind::vowel, .output = "ই", .dependent_output = "ি"},
    Definition{.input = "u", .kind = Kind::vowel, .output = "উ", .dependent_output = "ু"},
    Definition{.input = "e", .kind = Kind::vowel, .output = "এ", .dependent_output = "ে"},
    Definition{.input = "o", .kind = Kind::vowel, .output = "ও", .dependent_output = "ো"},
    Definition{.input = "w", .kind = Kind::vowel, .output = "ও", .dependent_output = "ো"},
    Definition{.input = "W", .kind = Kind::vowel, .output = "ও", .dependent_output = "ো"},
    Definition{.input = "x", .kind = Kind::cluster, .output = "ক্ষ", .dependent_output = {}},
    Definition{.input = "k", .kind = Kind::consonant, .output = "ক", .dependent_output = {}},
    Definition{.input = "q", .kind = Kind::consonant, .output = "ক", .dependent_output = {}},
    Definition{.input = "g", .kind = Kind::consonant, .output = "গ", .dependent_output = {}},
    Definition{.input = "c", .kind = Kind::consonant, .output = "চ", .dependent_output = {}},
    Definition{.input = "j", .kind = Kind::consonant, .output = "জ", .dependent_output = {}},
    Definition{.input = "z", .kind = Kind::consonant, .output = "জ", .dependent_output = {}},
    Definition{.input = "T", .kind = Kind::consonant, .output = "ট", .dependent_output = {}},
    Definition{.input = "D", .kind = Kind::consonant, .output = "ড", .dependent_output = {}},
    Definition{.input = "N", .kind = Kind::consonant, .output = "ণ", .dependent_output = {}},
    Definition{.input = "t", .kind = Kind::consonant, .output = "ত", .dependent_output = {}},
    Definition{.input = "d", .kind = Kind::consonant, .output = "দ", .dependent_output = {}},
    Definition{.input = "n", .kind = Kind::nasal_n, .output = "ন", .dependent_output = "ং"},
    Definition{.input = "p", .kind = Kind::consonant, .output = "প", .dependent_output = {}},
    Definition{.input = "f", .kind = Kind::consonant, .output = "ফ", .dependent_output = {}},
    Definition{.input = "b", .kind = Kind::consonant, .output = "ব", .dependent_output = {}},
    Definition{.input = "v", .kind = Kind::consonant, .output = "ভ", .dependent_output = {}},
    Definition{.input = "m", .kind = Kind::consonant, .output = "ম", .dependent_output = {}},
    Definition{.input = "y", .kind = Kind::consonant, .output = "য", .dependent_output = {}},
    Definition{.input = "Y", .kind = Kind::consonant, .output = "য়", .dependent_output = {}},
    Definition{.input = "r", .kind = Kind::consonant, .output = "র", .dependent_output = {}},
    Definition{.input = "R", .kind = Kind::consonant, .output = "ড়", .dependent_output = {}},
    Definition{.input = "l", .kind = Kind::consonant, .output = "ল", .dependent_output = {}},
    Definition{.input = "s", .kind = Kind::consonant, .output = "স", .dependent_output = {}},
    Definition{.input = "h", .kind = Kind::consonant, .output = "হ", .dependent_output = {}},
    Definition{.input = "M", .kind = Kind::anusvara, .output = "ং", .dependent_output = {}},
    Definition{.input = "H", .kind = Kind::visarga, .output = "ঃ", .dependent_output = {}},
    Definition{.input = "A", .kind = Kind::vowel, .output = "আ", .dependent_output = "া"},
    Definition{.input = "B", .kind = Kind::consonant, .output = "ব", .dependent_output = {}},
    Definition{.input = "C", .kind = Kind::consonant, .output = "চ", .dependent_output = {}},
    Definition{.input = "E", .kind = Kind::vowel, .output = "এ", .dependent_output = "ে"},
    Definition{.input = "F", .kind = Kind::consonant, .output = "ফ", .dependent_output = {}},
    Definition{.input = "G", .kind = Kind::consonant, .output = "গ", .dependent_output = {}},
    Definition{.input = "I", .kind = Kind::vowel, .output = "ই", .dependent_output = "ি"},
    Definition{.input = "J", .kind = Kind::consonant, .output = "জ", .dependent_output = {}},
    Definition{.input = "K", .kind = Kind::consonant, .output = "ক", .dependent_output = {}},
    Definition{.input = "L", .kind = Kind::consonant, .output = "ল", .dependent_output = {}},
    Definition{.input = "P", .kind = Kind::consonant, .output = "প", .dependent_output = {}},
    Definition{.input = "Q", .kind = Kind::consonant, .output = "ক", .dependent_output = {}},
    Definition{.input = "S", .kind = Kind::consonant, .output = "স", .dependent_output = {}},
    Definition{.input = "U", .kind = Kind::vowel, .output = "উ", .dependent_output = "ু"},
    Definition{.input = "V", .kind = Kind::consonant, .output = "ভ", .dependent_output = {}},
    Definition{.input = "X", .kind = Kind::cluster, .output = "ক্ষ", .dependent_output = {}},
    Definition{.input = "Z", .kind = Kind::consonant, .output = "জ", .dependent_output = {}},
};

}  // namespace

std::span<const TokenDefinition> token_registry() noexcept {
    return kTokens;
}

std::optional<TokenMatch> match_token(std::string_view input, std::size_t offset) noexcept {
    if (offset >= input.size()) {
        return std::nullopt;
    }

    const TokenDefinition* best_match = nullptr;
    for (const TokenDefinition& token : kTokens) {
        if (token.input.size() <= input.size() - offset
            && input.compare(offset, token.input.size(), token.input) == 0
            && (best_match == nullptr || token.input.size() > best_match->input.size())) {
            best_match = &token;
        }
    }

    if (best_match == nullptr) {
        return std::nullopt;
    }
    return TokenMatch{best_match, best_match->input.size()};
}

}  // namespace suborno::layouts::phonetic::detail
