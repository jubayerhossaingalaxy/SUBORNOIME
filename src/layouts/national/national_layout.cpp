#include "suborno/layouts/national/national_layout.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

struct NationalToken {
    std::string_view input;
    std::string_view independent;
    std::string_view dependent;
};

constexpr std::array kNationalTokens{
    // Clusters (longest match first)
    NationalToken{"kSh", "ক্ষ", {}},
    NationalToken{"GY", "জ্ঞ", {}},
    NationalToken{"JN", "জ্ঞ", {}},
    // Two-letter consonants
    NationalToken{"kh", "খ", {}},
    NationalToken{"gh", "ঘ", {}},
    NationalToken{"ch", "ছ", {}},
    NationalToken{"jh", "ঝ", {}},
    NationalToken{"Th", "ঠ", {}},
    NationalToken{"Dh", "ঢ", {}},
    NationalToken{"Ph", "ফ", {}},
    NationalToken{"ph", "ফ", {}},
    NationalToken{"Bh", "ভ", {}},
    NationalToken{"bh", "ভ", {}},
    NationalToken{"Sh", "ষ", {}},
    NationalToken{"Rh", "ঢ়", {}},
    // Single consonants
    NationalToken{"k", "ক", {}},
    NationalToken{"g", "গ", {}},
    NationalToken{"c", "চ", {}},
    NationalToken{"j", "জ", {}},
    NationalToken{"T", "ট", {}},
    NationalToken{"D", "ড", {}},
    NationalToken{"N", "ণ", {}},
    NationalToken{"t", "ত", {}},
    NationalToken{"th", "থ", {}},
    NationalToken{"d", "দ", {}},
    NationalToken{"dh", "ধ", {}},
    NationalToken{"n", "ন", "ং"},
    NationalToken{"p", "প", {}},
    NationalToken{"f", "ফ", {}},
    NationalToken{"b", "ব", {}},
    NationalToken{"v", "ভ", {}},
    NationalToken{"m", "ম", {}},
    NationalToken{"y", "য", {}},
    NationalToken{"r", "র", {}},
    NationalToken{"l", "ল", {}},
    NationalToken{"sh", "শ", {}},
    NationalToken{"s", "স", {}},
    NationalToken{"h", "হ", {}},
    NationalToken{"R", "ড়", {}},
    // Vowels: independent / dependent
    NationalToken{"a", "আ", "া"},
    NationalToken{"A", "অ", ""},
    NationalToken{"i", "ই", "ি"},
    NationalToken{"I", "ঈ", "ী"},
    NationalToken{"u", "উ", "ু"},
    NationalToken{"U", "ঊ", "ূ"},
    NationalToken{"e", "এ", "ে"},
    NationalToken{"ai", "ঐ", "ৈ"},
    NationalToken{"o", "ও", "ো"},
    NationalToken{"au", "ঔ", "ৌ"},
    // Signs
    NationalToken{"M", "ং", {}},
    NationalToken{"H", "ঃ", {}},
};

bool is_bengali_consonant(char32_t cp) {
    return cp >= 0x0995 && cp <= 0x09B9;
}

bool is_velar_token(std::string_view token) {
    return token == "k" || token == "kh" || token == "g" || token == "gh" || token == "K"
           || token == "Kh" || token == "G" || token == "Gh";
}

std::optional<std::string_view> match_next_token(std::string_view input, std::size_t offset) {
    if (offset >= input.size()) {
        return std::nullopt;
    }
    for (const auto& token : kNationalTokens) {
        if (token.input.size() <= input.size() - offset
            && input.compare(offset, token.input.size(), token.input) == 0) {
            return token.input;
        }
    }
    return std::nullopt;
}

std::string transliterate_national(std::string_view input) {
    std::string result;
    std::size_t offset = 0;
    bool after_consonant = false;

    while (offset < input.size()) {
        const NationalToken* best = nullptr;
        for (const auto& token : kNationalTokens) {
            if (token.input.size() <= input.size() - offset
                && input.compare(offset, token.input.size(), token.input) == 0
                && (best == nullptr || token.input.size() > best->input.size())) {
                best = &token;
            }
        }

        if (best == nullptr) {
            result.push_back(static_cast<char>(input[offset]));
            offset++;
            after_consonant = false;
            continue;
        }

        // Special handling for "n": dependent form (ং) before velar consonants,
        // independent form (ন) otherwise.
        if (best->input == "n") {
            const auto next = match_next_token(input, offset + best->input.size());
            if (next.has_value() && is_velar_token(*next)) {
                result.append(best->dependent);
            } else {
                result.append(best->independent);
            }
            after_consonant = true;
            offset += best->input.size();
            continue;
        }

        if (after_consonant && !best->dependent.empty()) {
            result.append(best->dependent);
        } else {
            result.append(best->independent);
        }

        const std::string_view out = best->independent;
        if (!out.empty()) {
            const unsigned char lead = static_cast<unsigned char>(out[0]);
            if (lead >= 0xE0 && lead <= 0xEF && out.size() >= 3) {
                char32_t cp = static_cast<char32_t>((out[0] & 0x0F) << 12 | (out[1] & 0x3F) << 6
                                                    | (out[2] & 0x3F));
                after_consonant = is_bengali_consonant(cp);
            } else {
                after_consonant = false;
            }
        } else {
            after_consonant = false;
        }

        offset += best->input.size();
    }

    return suborno::unicode::normalize_nfc(result);
}

}  // namespace

std::string NationalLayout::transliterate(const std::string& input) const {
    return transliterate_national(input);
}

std::vector<suborno::layout::Candidate> NationalLayout::generate_candidates(
    const std::string& input) const {
    const std::string output = transliterate_national(input);
    return {{output,
             output,
             "layout:national:" + output,
             suborno::layout::CandidateSource::layout,
             1.0,
             "national phonetic output"}};
}

}  // namespace suborno::layouts
