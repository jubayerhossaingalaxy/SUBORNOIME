#include "suborno/layouts/layout_input_session.hpp"

#include <algorithm>
#include <unordered_set>
#include <utility>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

constexpr double kPhoneticDictionaryScore = 0.9;

void append_dictionary_matches(std::vector<suborno::layout::Candidate>& candidates,
                              std::unordered_set<std::string>& seen,
                              const suborno::dictionary::Dictionary& dictionary,
                              const std::string& query,
                              double score,
                              const char* rationale,
                              std::size_t candidate_limit) {
    for (const std::string& word : dictionary.find_matches(query)) {
        if (candidates.size() >= candidate_limit) {
            break;
        }

        const std::string normalized_word = suborno::unicode::normalize_nfc(word);
        if (seen.insert(normalized_word).second) {
            candidates.push_back({
                .text = word,
                .normalized_text = normalized_word,
                .stable_id = "system-dictionary:" + normalized_word,
                .source = suborno::layout::CandidateSource::system_dictionary,
                .score = score,
                .rationale = rationale,
            });
        }
    }
}

}  // namespace

LayoutInputSession::LayoutInputSession(const suborno::layout::InputLayout& input_layout,
                                       suborno::layout::InputSessionOptions options)
    : input_layout_(&input_layout),
      options_(options) {}

LayoutInputSession::LayoutInputSession(const suborno::layout::InputLayout& input_layout,
                                       suborno::dictionary::Dictionary& dictionary,
                                       suborno::layout::InputSessionOptions options)
    : input_layout_(&input_layout),
      dictionary_(&dictionary),
      options_(options) {}

suborno::layout::InputStatus LayoutInputSession::append_utf8(std::string_view input) {
    if (!suborno::unicode::is_valid_utf8(input)) {
        return suborno::layout::InputStatus::invalid_utf8;
    }
    if (input.empty()) {
        return suborno::layout::InputStatus::empty;
    }

    std::string updated_input = raw_input_;
    updated_input.append(input);
    if (suborno::unicode::count_utf8_code_points(updated_input)
        > suborno::layout::InputSession::kMaxInputCodePoints) {
        return suborno::layout::InputStatus::input_limit;
    }

    raw_input_ = std::move(updated_input);
    selected_candidate_index_ = 0;
    return suborno::layout::InputStatus::updated;
}

suborno::layout::InputStatus LayoutInputSession::backspace() {
    if (raw_input_.empty()) {
        return suborno::layout::InputStatus::empty;
    }

    const auto graphemes = suborno::unicode::split_graphemes(raw_input_);
    if (graphemes.empty()) {
        raw_input_.clear();
        selected_candidate_index_ = 0;
        return suborno::layout::InputStatus::empty;
    }

    std::string updated_input = raw_input_;
    updated_input.resize(updated_input.size() - graphemes.back().size());
    raw_input_ = std::move(updated_input);
    selected_candidate_index_ = 0;
    return raw_input_.empty() ? suborno::layout::InputStatus::empty
                              : suborno::layout::InputStatus::updated;
}

std::vector<suborno::layout::Candidate> LayoutInputSession::make_candidates(
    const std::string& input) const {
    std::vector<suborno::layout::Candidate> candidates;
    std::unordered_set<std::string> seen;

    if (dictionary_ != nullptr && options_.candidate_limit > 0) {
        const std::string normalized =
            suborno::unicode::normalize_nfc(input_layout_->transliterate(input));
        append_dictionary_matches(candidates,
                                  seen,
                                  *dictionary_,
                                  normalized,
                                  1.0,
                                  "exact dictionary match",
                                  options_.candidate_limit);
        append_dictionary_matches(candidates,
                                  seen,
                                  *dictionary_,
                                  input,
                                  kPhoneticDictionaryScore,
                                  "input dictionary match",
                                  options_.candidate_limit);
    }

    for (const auto& candidate : input_layout_->generate_candidates(input)) {
        if (candidates.size() >= options_.candidate_limit) {
            break;
        }

        const std::string normalized_text =
            candidate.normalized_text.empty()
                ? suborno::unicode::normalize_nfc(candidate.text)
                : suborno::unicode::normalize_nfc(candidate.normalized_text);
        if (seen.insert(normalized_text).second) {
            suborno::layout::Candidate normalized_candidate = candidate;
            normalized_candidate.normalized_text = normalized_text;
            candidates.push_back(std::move(normalized_candidate));
        }
    }

    for (std::size_t index = 0; index < candidates.size(); ++index) {
        if (candidates[index].source == suborno::layout::CandidateSource::layout) {
            candidates[index].score = 1.0 / static_cast<double>(index + 1);
        }
    }
    return candidates;
}

suborno::layout::CommitResult LayoutInputSession::commit() {
    if (raw_input_.empty()) {
        return {
            .text_utf8 = {},
            .status = suborno::layout::InputStatus::empty,
        };
    }

    const auto current = snapshot();
    if (!current.candidates.empty()) {
        return commit_candidate(current.selected_candidate_index);
    }

    const std::string committed =
        suborno::unicode::normalize_nfc(input_layout_->transliterate(raw_input_));
    if (dictionary_ != nullptr && options_.learning_enabled && !committed.empty()) {
        dictionary_->record_commit(committed, committed);
    }

    raw_input_.clear();
    selected_candidate_index_ = 0;
    return {
        .text_utf8 = committed,
        .status = suborno::layout::InputStatus::updated,
    };
}

suborno::layout::CommitResult LayoutInputSession::commit_candidate(std::size_t index) {
    if (raw_input_.empty()) {
        return {
            .text_utf8 = {},
            .status = suborno::layout::InputStatus::empty,
        };
    }

    const auto candidates = make_candidates(raw_input_);
    if (index >= candidates.size()) {
        return {
            .text_utf8 = {},
            .status = suborno::layout::InputStatus::candidate_out_of_range,
        };
    }

    const auto& candidate = candidates[index];
    const std::string committed = candidate.text;
    if (dictionary_ != nullptr && options_.learning_enabled && !committed.empty()) {
        dictionary_->record_commit(committed,
                                   candidate.normalized_text.empty()
                                       ? committed
                                       : candidate.normalized_text);
    }

    raw_input_.clear();
    selected_candidate_index_ = 0;
    return {
        .text_utf8 = committed,
        .status = suborno::layout::InputStatus::updated,
    };
}

bool LayoutInputSession::select_candidate(std::size_t index) {
    if (index >= snapshot().candidates.size()) {
        return false;
    }
    selected_candidate_index_ = index;
    return true;
}

bool LayoutInputSession::select_next_candidate() {
    const std::size_t candidate_count = snapshot().candidates.size();
    if (selected_candidate_index_ + 1 >= candidate_count) {
        return false;
    }
    ++selected_candidate_index_;
    return true;
}

bool LayoutInputSession::select_previous_candidate() noexcept {
    if (selected_candidate_index_ == 0) {
        return false;
    }
    --selected_candidate_index_;
    return true;
}

std::size_t LayoutInputSession::selected_candidate_index() const noexcept {
    return selected_candidate_index_;
}

void LayoutInputSession::reset() noexcept {
    raw_input_.clear();
    selected_candidate_index_ = 0;
}

suborno::layout::CompositionSnapshot LayoutInputSession::snapshot() const {
    suborno::layout::CompositionSnapshot result;
    result.raw_input_utf8 = raw_input_;
    if (raw_input_.empty()) {
        result.status = suborno::layout::InputStatus::empty;
        return result;
    }

    result.preedit_utf8 =
        suborno::unicode::normalize_nfc(input_layout_->transliterate(raw_input_));
    result.status = suborno::layout::InputStatus::updated;
    result.candidates = make_candidates(raw_input_);

    if (selected_candidate_index_ >= result.candidates.size()) {
        selected_candidate_index_ =
            result.candidates.empty() ? 0 : result.candidates.size() - 1;
    }
    result.selected_candidate_index = selected_candidate_index_;
    return result;
}

}  // namespace suborno::layouts

namespace suborno::layout {

std::unique_ptr<InputSession> StatelessInputLayout::create_session(
    suborno::dictionary::Dictionary* dictionary,
    InputSessionOptions options) const {
    if (dictionary != nullptr) {
        return std::make_unique<suborno::layouts::LayoutInputSession>(
            *this, *dictionary, options);
    }
    return std::make_unique<suborno::layouts::LayoutInputSession>(*this, options);
}

}  // namespace suborno::layout
