#include "suborno/spell/spell_checker.hpp"

#include <algorithm>
#include <cmath>
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

}  // namespace

SpellChecker::SpellChecker(const dictionary::Dictionary& dictionary)
    : dictionary_(dictionary) {}

SpellingResult SpellChecker::check(const std::string& word) const {
    SpellingResult result;

    const auto exact_matches = dictionary_.find_matches(word);
    if (!exact_matches.empty()) {
        result.is_correct = true;
        return result;
    }

    // Generate edit-distance-1 candidates
    std::unordered_set<std::string> candidates;
    const auto edits1 = generate_edits(word);
    for (const auto& edit : edits1) {
        candidates.insert(edit);
    }

    // Edit distance 2 for common corrections
    for (const auto& edit : edits1) {
        const auto edits2 = generate_edits(edit);
        for (const auto& e2 : edits2) {
            candidates.insert(e2);
        }
    }

    // Score and rank suggestions
    struct ScoredCandidate {
        std::string word;
        double score;
    };
    std::vector<ScoredCandidate> scored;

    for (const auto& candidate : candidates) {
        const auto matches = dictionary_.find_matches(candidate);
        if (matches.empty()) {
            continue;
        }
        double dist = edit_distance(word, candidate);
        double freq_boost = 0.0;
        for (const auto& match : matches) {
            freq_boost += 1.0;
        }
        double score = freq_boost / (1.0 + dist);
        scored.push_back({matches.front(), score});
    }

    std::sort(scored.begin(), scored.end(), [](const ScoredCandidate& a, const ScoredCandidate& b) {
        return a.score > b.score;
    });

    // Limit to top 10 suggestions
    constexpr std::size_t kMaxSuggestions = 10;
    for (std::size_t i = 0; i < scored.size() && i < kMaxSuggestions; ++i) {
        result.suggestions.push_back({scored[i].word,
                                      scored[i].word,
                                      "spell:" + scored[i].word,
                                      suborno::layout::CandidateSource::user_dictionary,
                                      scored[i].score,
                                      "spelling suggestion"});
    }

    return result;
}

std::vector<std::string> SpellChecker::generate_edits(const std::string& word) const {
    std::vector<std::string> edits;

    const auto graphemes = suborno::unicode::split_graphemes(word);
    const auto replacements = bengali_replacement_graphemes();

    // Deletions
    for (std::size_t i = 0; i < graphemes.size(); ++i) {
        std::string result;
        for (std::size_t j = 0; j < graphemes.size(); ++j) {
            if (j != i) {
                result += graphemes[j];
            }
        }
        edits.push_back(std::move(result));
    }

    // Transpositions
    for (std::size_t i = 0; i + 1 < graphemes.size(); ++i) {
        std::string result;
        for (std::size_t j = 0; j < graphemes.size(); ++j) {
            if (j == i) {
                result += graphemes[i + 1];
            } else if (j == i + 1) {
                result += graphemes[i];
            } else {
                result += graphemes[j];
            }
        }
        edits.push_back(std::move(result));
    }

    // Replacements (using Bengali graphemes)
    for (std::size_t i = 0; i < graphemes.size(); ++i) {
        for (const auto& rep : replacements) {
            std::string result;
            for (std::size_t j = 0; j < graphemes.size(); ++j) {
                if (j == i) {
                    result += rep;
                } else {
                    result += graphemes[j];
                }
            }
            edits.push_back(std::move(result));
        }
    }

    // Insertions
    for (std::size_t i = 0; i <= graphemes.size(); ++i) {
        for (const auto& rep : replacements) {
            std::string result;
            for (std::size_t j = 0; j < graphemes.size(); ++j) {
                if (j == i) {
                    result += rep;
                }
                result += graphemes[j];
            }
            if (i == graphemes.size()) {
                result += rep;
            }
            edits.push_back(std::move(result));
        }
    }

    return edits;
}

double SpellChecker::edit_distance(const std::string& a, const std::string& b) const {
    const auto ga = suborno::unicode::split_graphemes(a);
    const auto gb = suborno::unicode::split_graphemes(b);

    const std::size_t m = ga.size();
    const std::size_t n = gb.size();

    if (m == 0)
        return static_cast<double>(n);
    if (n == 0)
        return static_cast<double>(m);

    std::vector<std::vector<double>> dp(m + 1, std::vector<double>(n + 1, 0.0));

    for (std::size_t i = 0; i <= m; ++i)
        dp[i][0] = static_cast<double>(i);
    for (std::size_t j = 0; j <= n; ++j)
        dp[0][j] = static_cast<double>(j);

    for (std::size_t i = 1; i <= m; ++i) {
        for (std::size_t j = 1; j <= n; ++j) {
            double cost = (ga[i - 1] == gb[j - 1]) ? 0.0 : 1.0;
            dp[i][j] = std::min({dp[i - 1][j] + 1.0,  // deletion
                                 dp[i][j - 1] + 1.0,  // insertion
                                 dp[i - 1][j - 1] + cost});  // substitution
        }
    }

    return dp[m][n];
}

}  // namespace suborno::spell
