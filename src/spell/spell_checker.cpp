#include "suborno/spell/spell_checker.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace bengali {

// Independent vowels (স্বরবর্ণ)
inline constexpr std::u32string_view vowels = U"অআইঈউঊঋঌএঐওঔৠৡ";

// Consonants (ব্যঞ্জনবর্ণ)
inline constexpr std::u32string_view consonants = U"কখগঘঙচছজঝঞটঠডঢণতথদধনপফবভমযরলশষসহ";

// Additional Bengali consonants (অতিরিক্ত ব্যঞ্জনবর্ণ)
inline constexpr std::u32string_view additional_consonants = U"ৎড়ঢ়য়";

// All letters usable as base replacement candidates
inline constexpr std::u32string_view replacements =
    U"অআইঈউঊঋঌএঐওঔৠৡ"
    U"কখগঘঙচছজঝঞটঠডঢণতথদধনপফবভমযরলশষসহ"
    U"ৎড়ঢ়য়";

// Dependent vowel signs (স্বরচিহ্ন / কার)
inline constexpr std::u32string_view vowel_signs = U"ািীুূৃৄেৈোৌৢৣ";

// Other Bengali signs (অন্যান্য চিহ্ন)
inline constexpr std::u32string_view signs = U"ঁংঃ়্ঽৗ৾";

// Bengali digits (বাংলা অঙ্ক)
inline constexpr std::u32string_view digits = U"০১২৩৪৫৬৭৮৯";

// Bengali punctuation (বাংলা দাঁড়ি ও দ্বি-দাঁড়ি)
inline constexpr std::u32string_view punctuation = U"।॥";

// Historical and special Bengali-block characters
inline constexpr std::u32string_view extended_symbols = U"ঀ৲৳৴৵৶৷৸৹৺৻ৼ৽";

}  // namespace bengali

namespace suborno::spell {

namespace {

constexpr std::size_t kMaximumWordGraphemes = 32;
constexpr std::size_t kMaximumGeneratedEdits = 512;
constexpr std::size_t kMaximumFirstPassLookups = 128;
constexpr std::size_t kMaximumSecondPassLookups = 128;
constexpr std::size_t kMaximumSecondPassSeeds = 16;
constexpr std::size_t kMaximumSuggestions = 10;

std::string to_utf8(char32_t code_point) {
    std::string result;
    if (code_point < 0x80) {
        result.push_back(static_cast<char>(code_point));
    } else if (code_point < 0x800) {
        result.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else if (code_point < 0x10000) {
        result.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        result.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else if (code_point < 0x110000) {
        result.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
        result.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
    return result;
}

std::vector<std::string> bengali_replacement_graphemes() {
    std::vector<std::string> result;
    result.reserve(bengali::replacements.size());
    for (char32_t cp : bengali::replacements) {
        result.push_back(to_utf8(cp));
    }
    return result;
}

std::vector<std::string> replacement_graphemes_for(
    const std::vector<std::string>& graphemes) {
    const bool is_ascii_latin_word =
        !graphemes.empty()
        && std::all_of(graphemes.begin(), graphemes.end(), [](const std::string& grapheme) {
               return grapheme.size() == 1
                      && ((grapheme[0] >= 'a' && grapheme[0] <= 'z')
                          || (grapheme[0] >= 'A' && grapheme[0] <= 'Z'));
           });
    if (!is_ascii_latin_word) {
        return bengali_replacement_graphemes();
    }

    const bool uppercase_word =
        std::all_of(graphemes.begin(), graphemes.end(), [](const std::string& grapheme) {
            return grapheme[0] >= 'A' && grapheme[0] <= 'Z';
        });
    std::vector<std::string> result;
    result.reserve(26);
    for (char letter = 'a'; letter <= 'z'; ++letter) {
        const char replacement =
            uppercase_word ? static_cast<char>(letter - ('a' - 'A')) : letter;
        result.emplace_back(1, replacement);
    }
    return result;
}

}  // namespace

SpellChecker::SpellChecker(const dictionary::Dictionary& dictionary)
    : dictionary_(dictionary) {}

SpellingResult SpellChecker::check(const std::string& word) const {
    SpellingResult result;
    if (word.empty() || !suborno::unicode::is_valid_utf8(word)) {
        return result;
    }

    const auto word_graphemes = suborno::unicode::split_graphemes(word);
    if (word_graphemes.empty() || word_graphemes.size() > kMaximumWordGraphemes) {
        return result;
    }

    const auto exact_matches = dictionary_.find_matches(word);
    if (!exact_matches.empty()) {
        result.is_correct = true;
        return result;
    }

    struct ScoredCandidate {
        std::string word;
        double score;
    };

    std::vector<ScoredCandidate> scored;
    std::unordered_set<std::string> checked_candidates;
    std::size_t dictionary_lookups = 0;

    const auto consider_candidate = [&](const std::string& candidate) {
        if (dictionary_lookups >= kMaximumFirstPassLookups + kMaximumSecondPassLookups
            || candidate.empty() || candidate == word
            || !checked_candidates.insert(candidate).second) {
            return;
        }

        ++dictionary_lookups;
        const auto matches = dictionary_.find_matches(candidate);
        if (matches.empty()) {
            return;
        }

        const double distance = edit_distance(word, candidate);
        const double frequency_boost = static_cast<double>(matches.size());
        scored.push_back({matches.front(), frequency_boost / (1.0 + distance)});
    };

    const auto edits1 = generate_edits(word);
    for (const auto& edit : edits1) {
        if (dictionary_lookups >= kMaximumFirstPassLookups) {
            break;
        }
        consider_candidate(edit);
    }

    for (std::size_t index = 0;
         index < edits1.size() && index < kMaximumSecondPassSeeds
         && dictionary_lookups < kMaximumFirstPassLookups + kMaximumSecondPassLookups;
         ++index) {
        const auto edits2 = generate_edits(edits1[index]);
        for (const auto& candidate : edits2) {
            if (dictionary_lookups >= kMaximumFirstPassLookups + kMaximumSecondPassLookups) {
                break;
            }
            consider_candidate(candidate);
        }
    }

    std::sort(scored.begin(), scored.end(), [](const ScoredCandidate& left,
                                               const ScoredCandidate& right) {
        if (left.score != right.score) {
            return left.score > right.score;
        }
        return left.word < right.word;
    });

    for (std::size_t index = 0;
         index < scored.size() && index < kMaximumSuggestions;
         ++index) {
        result.suggestions.push_back({scored[index].word,
                                      scored[index].word,
                                      "spell:" + scored[index].word,
                                      suborno::layout::CandidateSource::user_dictionary,
                                      scored[index].score,
                                      "spelling suggestion"});
    }

    return result;
}

std::vector<std::string> SpellChecker::generate_edits(const std::string& word) const {
    const auto graphemes = suborno::unicode::split_graphemes(word);
    if (graphemes.size() > kMaximumWordGraphemes) {
        return {};
    }

    const auto replacements = replacement_graphemes_for(graphemes);
    std::vector<std::string> edits;
    edits.reserve(kMaximumGeneratedEdits);
    std::unordered_set<std::string> seen;

    const auto append_edit = [&](std::string candidate) {
        if (edits.size() >= kMaximumGeneratedEdits) {
            return;
        }
        if (seen.insert(candidate).second) {
            edits.push_back(std::move(candidate));
        }
    };

    // Deletions
    for (std::size_t i = 0; i < graphemes.size() && edits.size() < kMaximumGeneratedEdits; ++i) {
        std::string candidate;
        for (std::size_t j = 0; j < graphemes.size(); ++j) {
            if (j != i) {
                candidate += graphemes[j];
            }
        }
        append_edit(std::move(candidate));
    }

    // Transpositions
    for (std::size_t i = 0;
         i + 1 < graphemes.size() && edits.size() < kMaximumGeneratedEdits;
         ++i) {
        std::string candidate;
        for (std::size_t j = 0; j < graphemes.size(); ++j) {
            if (j == i) {
                candidate += graphemes[i + 1];
            } else if (j == i + 1) {
                candidate += graphemes[i];
            } else {
                candidate += graphemes[j];
            }
        }
        append_edit(std::move(candidate));
    }

    // Interleave insertions and substitutions by position so both typo types
    // remain reachable within the first bounded dictionary lookup pass.
    for (std::size_t i = 0;
         i < graphemes.size() && edits.size() < kMaximumGeneratedEdits;
         ++i) {
        for (const auto& replacement : replacements) {
            if (edits.size() >= kMaximumGeneratedEdits) {
                break;
            }
            std::string candidate;
            for (std::size_t j = 0; j < graphemes.size(); ++j) {
                if (j == i) {
                    candidate += replacement;
                }
                candidate += graphemes[j];
            }
            append_edit(std::move(candidate));
        }

        for (const auto& replacement : replacements) {
            if (edits.size() >= kMaximumGeneratedEdits) {
                break;
            }
            std::string candidate;
            for (std::size_t j = 0; j < graphemes.size(); ++j) {
                candidate += j == i ? replacement : graphemes[j];
            }
            append_edit(std::move(candidate));
        }
    }

    // Insertions at the end of the word.
    for (const auto& replacement : replacements) {
        if (edits.size() >= kMaximumGeneratedEdits) {
            break;
        }
        std::string candidate;
        for (const auto& grapheme : graphemes) {
            candidate += grapheme;
        }
        candidate += replacement;
        append_edit(std::move(candidate));
    }

    return edits;
}

double SpellChecker::edit_distance(const std::string& a, const std::string& b) const {
    auto graphemes_a = suborno::unicode::split_graphemes(a);
    auto graphemes_b = suborno::unicode::split_graphemes(b);
    if (graphemes_a.size() > kMaximumWordGraphemes
        || graphemes_b.size() > kMaximumWordGraphemes) {
        throw std::length_error("Spell-check word exceeds the configured grapheme limit");
    }

    if (graphemes_a.size() < graphemes_b.size()) {
        graphemes_a.swap(graphemes_b);
    }

    std::vector<std::size_t> previous(graphemes_b.size() + 1);
    std::vector<std::size_t> current(graphemes_b.size() + 1);
    std::iota(previous.begin(), previous.end(), 0);

    for (std::size_t i = 1; i <= graphemes_a.size(); ++i) {
        current[0] = i;
        for (std::size_t j = 1; j <= graphemes_b.size(); ++j) {
            const std::size_t substitution_cost =
                graphemes_a[i - 1] == graphemes_b[j - 1] ? 0U : 1U;
            current[j] = std::min({previous[j] + 1,
                                   current[j - 1] + 1,
                                   previous[j - 1] + substitution_cost});
        }
        previous.swap(current);
    }

    return static_cast<double>(previous.back());
}

}  // namespace suborno::spell
