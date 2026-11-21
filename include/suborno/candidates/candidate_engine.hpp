#pragma once

#include <string>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::candidates {

class CandidateEngine {
  public:
    explicit CandidateEngine(const layout::PhoneticLayout& layout);
    CandidateEngine(const layout::PhoneticLayout& layout, const dictionary::Dictionary& dictionary);

    std::vector<layout::Candidate> make_candidates(const std::string& input) const;

  private:
    const layout::PhoneticLayout& layout_;
    const dictionary::Dictionary* dictionary_ = nullptr;
};

}  // namespace suborno::candidates
