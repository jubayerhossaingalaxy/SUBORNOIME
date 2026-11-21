#include "suborno/candidates/language_intelligence.hpp"

#include "suborno/dictionary/dictionary.hpp"

namespace suborno::candidates {

LanguageIntelligence::LanguageIntelligence(const suborno::dictionary::Dictionary& dictionary)
    : dictionary_(dictionary) {}

bool LanguageIntelligence::is_known_word(const std::string& normalized_word) const {
    return !dictionary_.find_matches(normalized_word).empty();
}

std::vector<std::string> LanguageIntelligence::suggest_prefix(const std::string& prefix) const {
    return dictionary_.words_for_prefix(prefix);
}

std::string LanguageIntelligence::correct_or_original(const std::string& normalized_word) const {
    const auto matches = dictionary_.find_matches(normalized_word);
    return matches.empty() ? normalized_word : matches.front();
}

}  // namespace suborno::candidates
