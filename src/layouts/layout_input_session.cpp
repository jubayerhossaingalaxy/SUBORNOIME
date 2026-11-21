#include "suborno/layouts/layout_input_session.hpp"

#include <utility>

#include "suborno/candidates/candidate_engine.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
LayoutInputSession::LayoutInputSession(const suborno::layout::InputLayout& input_layout,
                                       suborno::layout::InputSessionOptions options)
    : input_layout_(&input_layout),
      options_(options),
      pending_learning_enabled_(options.learning_enabled) {}

LayoutInputSession::LayoutInputSession(const suborno::layout::InputLayout& input_layout,
                                       suborno::dictionary::Dictionary& dictionary,
                                       suborno::layout::InputSessionOptions options)
    : input_layout_(&input_layout),
      dictionary_(&dictionary),
      options_(options),
      pending_learning_enabled_(options.learning_enabled) {}

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
        options_.learning_enabled = pending_learning_enabled_;
        return suborno::layout::InputStatus::empty;
    }

    std::string updated_input = raw_input_;
    updated_input.resize(updated_input.size() - graphemes.back().size());
    raw_input_ = std::move(updated_input);
    selected_candidate_index_ = 0;
    if (raw_input_.empty()) {
        options_.learning_enabled = pending_learning_enabled_;
        return suborno::layout::InputStatus::empty;
    }
    return suborno::layout::InputStatus::updated;
}

std::vector<suborno::layout::Candidate> LayoutInputSession::make_candidates(
    const std::string& input) const {
    const suborno::candidates::CandidateEngine candidate_engine(*input_layout_, dictionary_);
    return candidate_engine.make_candidates(input, options_.candidate_limit);
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
        dictionary_->record_commit(committed, committed, raw_input_);
    }

    raw_input_.clear();
    selected_candidate_index_ = 0;
    options_.learning_enabled = pending_learning_enabled_;
    return {
        .text_utf8 = committed,
        .status = suborno::layout::InputStatus::updated,
    };
}

void LayoutInputSession::set_learning_enabled(bool enabled) noexcept {
    pending_learning_enabled_ = enabled;
    if (raw_input_.empty() || !enabled) {
        options_.learning_enabled = enabled;
    }
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
        dictionary_->record_commit(
            committed,
            candidate.normalized_text.empty() ? committed : candidate.normalized_text,
            raw_input_);
    }

    raw_input_.clear();
    selected_candidate_index_ = 0;
    options_.learning_enabled = pending_learning_enabled_;
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
    options_.learning_enabled = pending_learning_enabled_;
}

suborno::layout::CompositionSnapshot LayoutInputSession::snapshot() const {
    suborno::layout::CompositionSnapshot result;
    result.raw_input_utf8 = raw_input_;
    if (raw_input_.empty()) {
        result.status = suborno::layout::InputStatus::empty;
        return result;
    }

    result.preedit_utf8 = suborno::unicode::normalize_nfc(input_layout_->transliterate(raw_input_));
    result.status = suborno::layout::InputStatus::updated;
    result.candidates = make_candidates(raw_input_);

    if (selected_candidate_index_ >= result.candidates.size()) {
        selected_candidate_index_ = result.candidates.empty() ? 0 : result.candidates.size() - 1;
    }
    result.selected_candidate_index = selected_candidate_index_;
    return result;
}

}  // namespace suborno::layouts

namespace suborno::layout {

std::unique_ptr<InputSession> StatelessInputLayout::create_session(
    suborno::dictionary::Dictionary* dictionary, InputSessionOptions options) const {
    if (dictionary != nullptr) {
        return std::make_unique<suborno::layouts::LayoutInputSession>(*this, *dictionary, options);
    }
    return std::make_unique<suborno::layouts::LayoutInputSession>(*this, options);
}

}  // namespace suborno::layout
