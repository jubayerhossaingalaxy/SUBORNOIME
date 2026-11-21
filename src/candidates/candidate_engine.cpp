#include "suborno/candidates/candidate_engine.hpp"

#include <string>
#include <unordered_set>
#include <utility>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::candidates {

CandidateEngine::CandidateEngine(const suborno::layout::InputLayout& layout,
                                 const suborno::dictionary::Dictionary* dictionary)
    : layout_(layout),
      dictionary_(dictionary) {}

CandidateEngine::CandidateEngine(const suborno::layout::InputLayout& layout,
                                 const suborno::dictionary::Dictionary& dictionary)
    : CandidateEngine(layout, &dictionary) {}

std::vector<suborno::layout::Candidate> CandidateEngine::make_candidates(
    std::string_view input, std::size_t candidate_limit) const {
    std::vector<suborno::layout::Candidate> result;
    if (candidate_limit == 0) {
        return result;
    }

    std::unordered_set<std::string> seen;
    const std::string input_text(input);

    const auto append_dictionary_matches =
        [&](const std::string& query, double score, const char* rationale) {
            if (dictionary_ == nullptr || result.size() >= candidate_limit) {
                return;
            }

            const auto matches = dictionary_->find_matches(query, candidate_limit);
            for (const std::string& word : matches) {
                if (result.size() >= candidate_limit) {
                    break;
                }

                const std::string normalized_word = suborno::unicode::normalize_nfc(word);
                if (seen.insert(normalized_word).second) {
                    result.push_back({
                        .text = word,
                        .normalized_text = normalized_word,
                        .stable_id = "system-dictionary:" + normalized_word,
                        .source = suborno::layout::CandidateSource::system_dictionary,
                        .score = score,
                        .rationale = rationale,
                    });
                }
            }
        };

    if (dictionary_ != nullptr) {
        const std::string normalized_input =
            suborno::unicode::normalize_nfc(layout_.transliterate(input_text));
        append_dictionary_matches(normalized_input, 1.0, "exact dictionary match");
        if (input_text != normalized_input) {
            append_dictionary_matches(input_text, 0.9, "input dictionary match");
        }

        const auto append_prefix_matches = [&](const std::string& query,
                                                double score,
                                                const char* rationale) {
            if (result.size() >= candidate_limit) {
                return;
            }
            const std::size_t remaining = candidate_limit - result.size();
            const auto matches = dictionary_->find_prefix_matches(query, remaining);
            for (const std::string& word : matches) {
                if (result.size() >= candidate_limit) {
                    break;
                }
                const std::string normalized_word = suborno::unicode::normalize_nfc(word);
                if (seen.insert(normalized_word).second) {
                    result.push_back({
                        .text = word,
                        .normalized_text = normalized_word,
                        .stable_id = "dictionary-prefix:" + normalized_word,
                        .source = suborno::layout::CandidateSource::system_dictionary,
                        .score = score,
                        .rationale = rationale,
                    });
                }
            }
        };

        append_prefix_matches(input_text, 0.75, "normalized input prefix match");
        if (normalized_input != input_text) {
            append_prefix_matches(normalized_input, 0.7, "transliterated prefix match");
        }
    }

    if (result.size() < candidate_limit) {
        for (const auto& candidate : layout_.generate_candidates(input_text)) {
            if (result.size() >= candidate_limit) {
                break;
            }

            const std::string normalized_text =
                candidate.normalized_text.empty()
                    ? suborno::unicode::normalize_nfc(candidate.text)
                    : suborno::unicode::normalize_nfc(candidate.normalized_text);
            if (seen.insert(normalized_text).second) {
                suborno::layout::Candidate normalized_candidate = candidate;
                normalized_candidate.normalized_text = normalized_text;
                result.push_back(std::move(normalized_candidate));
            }
        }
    }

    for (std::size_t index = 0; index < result.size(); ++index) {
        if (result[index].source == suborno::layout::CandidateSource::layout) {
            result[index].score = 1.0 / static_cast<double>(index + 1);
        }
    }

    return result;
}

}  // namespace suborno::candidates
