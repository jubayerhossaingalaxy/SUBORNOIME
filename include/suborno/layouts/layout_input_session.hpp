#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "suborno/layout-api/input_layout.hpp"
#include "suborno/layout-api/input_session.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::layouts {

class LayoutInputSession final : public layout::InputSession {
  public:
    LayoutInputSession(const layout::InputLayout& input_layout,
                       layout::InputSessionOptions options = {});
    LayoutInputSession(const layout::InputLayout& input_layout,
                       dictionary::Dictionary& dictionary,
                       layout::InputSessionOptions options = {});

    layout::InputStatus append_utf8(std::string_view input) override;
    layout::InputStatus backspace() override;
    layout::CommitResult commit() override;
    layout::CommitResult commit_candidate(std::size_t index) override;
    void set_learning_enabled(bool enabled) noexcept override;
    bool select_candidate(std::size_t index) override;
    bool select_next_candidate() override;
    bool select_previous_candidate() noexcept override;
    std::size_t selected_candidate_index() const noexcept override;
    void reset() noexcept override;
    layout::CompositionSnapshot snapshot() const override;

  private:
    std::vector<layout::Candidate> make_candidates(const std::string& input) const;

    const layout::InputLayout* input_layout_ = nullptr;
    dictionary::Dictionary* dictionary_ = nullptr;
    layout::InputSessionOptions options_;
    bool pending_learning_enabled_ = false;
    std::string raw_input_;
    mutable std::size_t selected_candidate_index_ = 0;
};

}  // namespace suborno::layouts
