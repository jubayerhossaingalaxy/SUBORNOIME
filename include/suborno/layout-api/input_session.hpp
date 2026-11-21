#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "suborno/layout-api/candidate.hpp"
#include "suborno/layout-api/key_event.hpp"

namespace suborno::layout {

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
    std::vector<Candidate> candidates;
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

class InputSession {
  public:
    static constexpr std::size_t kMaxInputCodePoints = 256;

    virtual ~InputSession() = default;

    virtual InputStatus append_utf8(std::string_view input) = 0;

    // The default maps a physical key event to its US-layout ASCII character.
    // Fixed-layout sessions may override this to use the physical key directly.
    virtual InputStatus append_key(const KeyEvent& event) {
        const std::optional<char> character = key_event_to_ascii(event);
        if (!character.has_value()) {
            return InputStatus::empty;
        }
        const char value = *character;
        return append_utf8(std::string_view(&value, 1));
    }

    virtual InputStatus backspace() = 0;
    virtual CommitResult commit() = 0;
    virtual CommitResult commit_candidate(std::size_t index) = 0;
    virtual bool select_candidate(std::size_t index) = 0;
    virtual bool select_next_candidate() = 0;
    virtual bool select_previous_candidate() noexcept = 0;
    virtual std::size_t selected_candidate_index() const noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual CompositionSnapshot snapshot() const = 0;
};

}  // namespace suborno::layout
