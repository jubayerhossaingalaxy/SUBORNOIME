#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "suborno/layout-api/input_layout.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::candidates {

class CandidateEngine {
  public:
    explicit CandidateEngine(const layout::InputLayout& layout,
                             const dictionary::Dictionary* dictionary = nullptr);
    CandidateEngine(const layout::InputLayout& layout, const dictionary::Dictionary& dictionary);

    std::vector<layout::Candidate> make_candidates(std::string_view input,
                                                   std::size_t candidate_limit = 16) const;

  private:
    const layout::InputLayout& layout_;
    const dictionary::Dictionary* dictionary_ = nullptr;
};

}  // namespace suborno::candidates
