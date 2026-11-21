#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "suborno/layout-api/input_session.hpp"
#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::layouts {

class PhoneticInputSession final : public layout::InputSession {
  public:
    static constexpr std::size_t kMaxInputCodePoints = layout::InputSession::kMaxInputCodePoints;

    PhoneticInputSession();
    explicit PhoneticInputSession(const layout::PhoneticLayout& layout,
                                  layout::InputSessionOptions options = {});
    PhoneticInputSession(const layout::PhoneticLayout& layout,
                         dictionary::Dictionary& dictionary,
                         layout::InputSessionOptions options = {});

    // Each session owns mutable composition state and is thread-confined.
    layout::InputStatus append_utf8(std::string_view input) override;
    layout::InputStatus backspace() override;
    layout::CommitResult commit() override;
    layout::CommitResult commit_candidate(std::size_t index) override;
    bool select_candidate(std::size_t index) override;
    bool select_next_candidate() override;
    bool select_previous_candidate() noexcept override;
    std::size_t selected_candidate_index() const noexcept override;
    void reset() noexcept override;
    layout::CompositionSnapshot snapshot() const override;

  private:
    const layout::PhoneticLayout* layout_ = nullptr;
    dictionary::Dictionary* dictionary_ = nullptr;
    layout::InputSessionOptions options_;
    std::string raw_input_;
    mutable std::size_t selected_candidate_index_ = 0;
};

}  // namespace suborno::layouts
