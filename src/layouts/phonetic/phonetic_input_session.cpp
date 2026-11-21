#include "suborno/layouts/phonetic/phonetic_input_session.hpp"

#include <utility>

#include "phonetic_parser.hpp"
#include "suborno/candidates/candidate_engine.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layouts/layout_input_session.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {

PhoneticInputSession::PhoneticInputSession() {
    static const SubornoPhoneticLayout default_layout;
    layout_ = &default_layout;
}

PhoneticInputSession::PhoneticInputSession(const suborno::layout::PhoneticLayout& layout,
                                           InputSessionOptions options)
    : layout_(&layout),
      options_(options) {}

PhoneticInputSession::PhoneticInputSession(const suborno::layout::PhoneticLayout& layout,
                                           suborno::dictionary::Dictionary& dictionary,
                                           InputSessionOptions options)
    : layout_(&layout),
      dictionary_(&dictionary),
      options_(options) {}

InputStatus PhoneticInputSession::append_utf8(std::string_view input) {
    if (!suborno::unicode::is_valid_utf8(input)) {
        return InputStatus::invalid_utf8;
    }
    if (input.empty()) {
        return InputStatus::empty;
    }

    std::string updated_input = raw_input_;
    updated_input.append(input);
    if (suborno::unicode::count_utf8_code_points(updated_input) > kMaxInputCodePoints) {
        return InputStatus::input_limit;
    }

    const auto parsed = suborno::layouts::phonetic::detail::parse_primary(updated_input, false);
    raw_input_ = std::move(updated_input);
    selected_candidate_index_ = 0;
    if (parsed.pending) {
        return InputStatus::pending;
    }
    return parsed.literal_passthrough ? InputStatus::literal_passthrough : InputStatus::updated;
}

InputStatus PhoneticInputSession::backspace() {
    if (raw_input_.empty()) {
        return InputStatus::empty;
    }

    const auto graphemes = suborno::unicode::split_graphemes(raw_input_);
    std::string updated_input = raw_input_;
    updated_input.resize(updated_input.size() - graphemes.back().size());
    const auto parsed = suborno::layouts::phonetic::detail::parse_primary(updated_input, false);
    raw_input_ = std::move(updated_input);
    selected_candidate_index_ = 0;
    if (raw_input_.empty()) {
        return InputStatus::empty;
    }
    if (parsed.pending) {
        return InputStatus::pending;
    }
    return parsed.literal_passthrough ? InputStatus::literal_passthrough : InputStatus::updated;
}

CommitResult PhoneticInputSession::commit() {
    if (raw_input_.empty()) {
        return {
            .text_utf8 = {},
            .status = InputStatus::empty,
        };
    }

    const CompositionSnapshot current = snapshot();
    if (!current.candidates.empty()) {
        return commit_candidate(current.selected_candidate_index);
    }

    const auto parsed = suborno::layouts::phonetic::detail::parse_primary(raw_input_, true);
    const std::string committed =
        parsed.literal_passthrough ? parsed.text : layout_->transliterate(raw_input_);
    if (dictionary_ != nullptr && options_.learning_enabled) {
        dictionary_->record_commit(committed, raw_input_);
    }
    raw_input_.clear();
    selected_candidate_index_ = 0;
    return {
        .text_utf8 = committed,
        .status =
            parsed.literal_passthrough ? InputStatus::literal_passthrough : InputStatus::updated,
    };
}

CommitResult PhoneticInputSession::commit_candidate(std::size_t index) {
    if (raw_input_.empty()) {
        return {
            .text_utf8 = {},
            .status = InputStatus::empty,
        };
    }

    const suborno::candidates::CandidateEngine candidate_engine(*layout_, dictionary_);
    const auto candidates = candidate_engine.make_candidates(raw_input_, options_.candidate_limit);
    if (index >= candidates.size()) {
        return {
            .text_utf8 = {},
            .status = InputStatus::candidate_out_of_range,
        };
    }
    selected_candidate_index_ = index;

    const auto parsed = suborno::layouts::phonetic::detail::parse_primary(raw_input_, true);
    const std::string committed = candidates[index].text;
    if (dictionary_ != nullptr && options_.learning_enabled) {
        dictionary_->record_commit(committed, raw_input_);
    }
    raw_input_.clear();
    selected_candidate_index_ = 0;
    return {
        .text_utf8 = committed,
        .status =
            parsed.literal_passthrough ? InputStatus::literal_passthrough : InputStatus::updated,
    };
}

bool PhoneticInputSession::select_candidate(std::size_t index) {
    const std::size_t candidate_count = snapshot().candidates.size();
    if (index >= candidate_count) {
        return false;
    }
    selected_candidate_index_ = index;
    return true;
}

bool PhoneticInputSession::select_next_candidate() {
    const std::size_t candidate_count = snapshot().candidates.size();
    if (candidate_count == 0) {
        return false;
    }
    if (selected_candidate_index_ + 1 >= candidate_count) {
        return false;
    }
    ++selected_candidate_index_;
    return true;
}

bool PhoneticInputSession::select_previous_candidate() noexcept {
    if (selected_candidate_index_ == 0) {
        return false;
    }
    --selected_candidate_index_;
    return true;
}

std::size_t PhoneticInputSession::selected_candidate_index() const noexcept {
    return selected_candidate_index_;
}

void PhoneticInputSession::reset() noexcept {
    raw_input_.clear();
    selected_candidate_index_ = 0;
}

CompositionSnapshot PhoneticInputSession::snapshot() const {
    CompositionSnapshot result;
    result.raw_input_utf8 = raw_input_;
    if (raw_input_.empty()) {
        result.status = InputStatus::empty;
        return result;
    }

    const auto parsed = suborno::layouts::phonetic::detail::parse_primary(raw_input_, false);
    result.preedit_utf8 = (parsed.pending || parsed.literal_passthrough)
                              ? parsed.text
                              : layout_->transliterate(raw_input_);
    if (parsed.pending) {
        result.status = InputStatus::pending;
    } else if (parsed.literal_passthrough) {
        result.status = InputStatus::literal_passthrough;
    } else {
        result.status = InputStatus::updated;
    }
    if (!parsed.pending) {
        const suborno::candidates::CandidateEngine candidate_engine(*layout_, dictionary_);
        result.candidates = candidate_engine.make_candidates(raw_input_, options_.candidate_limit);
        if (selected_candidate_index_ >= result.candidates.size()) {
            selected_candidate_index_ =
                result.candidates.empty() ? 0 : result.candidates.size() - 1;
        }
        result.selected_candidate_index = selected_candidate_index_;
    }
    return result;
}

}  // namespace suborno::layouts

namespace suborno::layout {

std::unique_ptr<InputSession> PhoneticLayout::create_session(
    suborno::dictionary::Dictionary* dictionary, InputSessionOptions options) const {
    if (dictionary != nullptr) {
        return std::make_unique<suborno::layouts::PhoneticInputSession>(
            *this, *dictionary, options);
    }
    return std::make_unique<suborno::layouts::PhoneticInputSession>(*this, options);
}

std::unique_ptr<InputSession> StatelessPhoneticLayout::create_session(
    suborno::dictionary::Dictionary* dictionary, InputSessionOptions options) const {
    if (dictionary != nullptr) {
        return std::make_unique<suborno::layouts::LayoutInputSession>(*this, *dictionary, options);
    }
    return std::make_unique<suborno::layouts::LayoutInputSession>(*this, options);
}

}  // namespace suborno::layout
