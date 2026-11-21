#include "suborno/layouts/probhat/probhat_layout.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

struct ProbhatToken {
    std::string_view input;
    std::string_view independent;
    std::string_view dependent;
};

constexpr std::array kProbhatTokens{
    // Clusters (longest match first)
    ProbhatToken{"kSh", "ক্ষ", {}},
    ProbhatToken{"GY", "জ্ঞ", {}},
    ProbhatToken{"JN", "জ্ঞ", {}},
    // Two-letter consonants
    ProbhatToken{"kh", "খ", {}},
    ProbhatToken{"gh", "ঘ", {}},
    ProbhatToken{"ch", "ছ", {}},
    ProbhatToken{"jh", "ঝ", {}},
    ProbhatToken{"Th", "ঠ", {}},
    ProbhatToken{"Dh", "ঢ", {}},
    ProbhatToken{"Ph", "ফ", {}},
    ProbhatToken{"ph", "ফ", {}},
    ProbhatToken{"Bh", "ভ", {}},
    ProbhatToken{"bh", "ভ", {}},
    ProbhatToken{"Sh", "ষ", {}},
    ProbhatToken{"Rh", "ঢ়", {}},
    // Single consonants
    ProbhatToken{"k", "ক", {}},
    ProbhatToken{"g", "গ", {}},
    ProbhatToken{"c", "চ", {}},
    ProbhatToken{"j", "জ", {}},
    ProbhatToken{"T", "ট", {}},
    ProbhatToken{"D", "ড", {}},
    ProbhatToken{"N", "ণ", {}},
    ProbhatToken{"t", "ত", {}},
    ProbhatToken{"th", "থ", {}},
    ProbhatToken{"d", "দ", {}},
    ProbhatToken{"dh", "ধ", {}},
    ProbhatToken{"n", "ন", "ং"},
    ProbhatToken{"p", "প", {}},
    ProbhatToken{"f", "ফ", {}},
    ProbhatToken{"b", "ব", {}},
    ProbhatToken{"v", "ভ", {}},
    ProbhatToken{"m", "ম", {}},
    ProbhatToken{"y", "য", {}},
    ProbhatToken{"r", "র", {}},
    ProbhatToken{"l", "ল", {}},
    ProbhatToken{"sh", "শ", {}},
    ProbhatToken{"s", "স", {}},
    ProbhatToken{"h", "হ", {}},
    ProbhatToken{"R", "ড়", {}},
    // Vowels: independent / dependent
    ProbhatToken{"a", "আ", "া"},
    ProbhatToken{"A", "অ", ""},
    ProbhatToken{"i", "ই", "ি"},
    ProbhatToken{"I", "ঈ", "ী"},
    ProbhatToken{"u", "উ", "ু"},
    ProbhatToken{"U", "ঊ", "ূ"},
    ProbhatToken{"e", "এ", "ে"},
    ProbhatToken{"ai", "ঐ", "ৈ"},
    ProbhatToken{"o", "ও", "ো"},
    ProbhatToken{"au", "ঔ", "ৌ"},
    // Signs
    ProbhatToken{"M", "ং", {}},
    ProbhatToken{"H", "ঃ", {}},
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
    for (const auto& token : kProbhatTokens) {
        if (token.input.size() <= input.size() - offset
            && input.compare(offset, token.input.size(), token.input) == 0) {
            return token.input;
        }
    }
    return std::nullopt;
}

std::string transliterate_probhat(std::string_view input) {
    std::string result;
    std::size_t offset = 0;
    bool after_consonant = false;

    while (offset < input.size()) {
        const ProbhatToken* best = nullptr;
        for (const auto& token : kProbhatTokens) {
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

        // Track consonant state for dependent vowel selection
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

std::string ProbhatLayout::transliterate(const std::string& input) const {
    return transliterate_probhat(input);
}

std::vector<suborno::layout::Candidate> ProbhatLayout::generate_candidates(
    const std::string& input) const {
    const std::string output = transliterate_probhat(input);
    return {{output,
             output,
             "layout:probhat:" + output,
             suborno::layout::CandidateSource::layout,
             1.0,
             "probhat phonetic output"}};
}

}  // namespace suborno::layouts
