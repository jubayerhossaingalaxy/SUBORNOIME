#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::layouts {

enum class InputStatus {
    updated,
    pending,
    literal_passthrough,
    invalid_utf8,
    input_limit,
    candidate_out_of_range,
    empty,
};

struct CompositionSnapshot {
    std::string raw_input_utf8;
    std::string preedit_utf8;
    std::vector<layout::Candidate> candidates;
    std::size_t selected_candidate_index = 0;
    InputStatus status = InputStatus::empty;
};

struct CommitResult {
    std::string text_utf8;
    InputStatus status = InputStatus::empty;
};

struct InputSessionOptions {
    std::size_t candidate_limit = 16;
    bool learning_enabled = true;
};

class PhoneticInputSession final {
  public:
    static constexpr std::size_t kMaxInputCodePoints = 256;

    PhoneticInputSession();
    explicit PhoneticInputSession(const layout::PhoneticLayout& layout,
                                  InputSessionOptions options = {});
    PhoneticInputSession(const layout::PhoneticLayout& layout,
                         dictionary::Dictionary& dictionary,
                         InputSessionOptions options = {});

    // Each session owns mutable composition state and is thread-confined.
    InputStatus append_utf8(std::string_view input);
    InputStatus backspace();
    CommitResult commit();
    CommitResult commit_candidate(std::size_t index);
    bool select_candidate(std::size_t index);
    bool select_next_candidate();
    bool select_previous_candidate() noexcept;
    std::size_t selected_candidate_index() const noexcept;
    void reset() noexcept;
    CompositionSnapshot snapshot() const;

  private:
    const layout::PhoneticLayout* layout_ = nullptr;
    dictionary::Dictionary* dictionary_ = nullptr;
    InputSessionOptions options_;
    std::string raw_input_;
    mutable std::size_t selected_candidate_index_ = 0;
};

}  // namespace suborno::layouts
