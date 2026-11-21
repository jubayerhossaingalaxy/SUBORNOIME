#include "suborno/candidates/candidate_engine.hpp"

#include <unordered_set>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::candidates {
namespace {

constexpr double kPhoneticMatchScore = 0.9;

}  // namespace

CandidateEngine::CandidateEngine(const suborno::layout::PhoneticLayout& layout)
    : layout_(layout) {}
CandidateEngine::CandidateEngine(const suborno::layout::PhoneticLayout& layout,
                                 const suborno::dictionary::Dictionary& dictionary)
    : layout_(layout),
      dictionary_(&dictionary) {}

std::vector<suborno::layout::Candidate> CandidateEngine::make_candidates(
    const std::string& input) const {
    std::vector<suborno::layout::Candidate> result;
    std::unordered_set<std::string> seen;

    const std::string normalized = suborno::unicode::normalize_nfc(layout_.transliterate(input));

    if (dictionary_ != nullptr) {
        for (const std::string& word : dictionary_->find_matches(normalized)) {
            const std::string normalized_word = suborno::unicode::normalize_nfc(word);
            if (seen.insert(normalized_word).second) {
                result.push_back({
                    .text = word,
                    .normalized_text = normalized_word,
                    .stable_id = "system-dictionary:" + normalized_word,
                    .source = suborno::layout::CandidateSource::system_dictionary,
                    .score = 1.0,
                    .rationale = "exact dictionary match",
                });
            }
        }
        for (const std::string& word : dictionary_->find_matches(input)) {
            const std::string normalized_word = suborno::unicode::normalize_nfc(word);
            if (seen.insert(normalized_word).second) {
                result.push_back({
                    .text = word,
                    .normalized_text = normalized_word,
                    .stable_id = "system-dictionary:" + normalized_word,
                    .source = suborno::layout::CandidateSource::system_dictionary,
                    .score = kPhoneticMatchScore,
                    .rationale = "phonetic dictionary match",
                });
            }
        }
    }

    for (const auto& candidate : layout_.generate_candidates(input)) {
        const std::string normalized_text =
            candidate.normalized_text.empty()
                ? suborno::unicode::normalize_nfc(candidate.text)
                : suborno::unicode::normalize_nfc(candidate.normalized_text);
        if (seen.insert(normalized_text).second) {
            result.push_back(candidate);
        }
    }

    for (std::size_t index = 0; index < result.size(); ++index) {
        if (result[index].rationale.find("dictionary") == std::string::npos) {
            result[index].score = 1.0 / static_cast<double>(index + 1);
        }
    }
    return result;
}

}  // namespace suborno::candidates
