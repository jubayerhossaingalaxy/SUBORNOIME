#pragma once

#include <string>
#include <vector>

namespace suborno::layout {

enum class CandidateSource {
    layout,
    system_dictionary,
    user_dictionary,
};

struct Candidate {
    std::string text;
    std::string normalized_text;
    std::string stable_id;
    CandidateSource source = CandidateSource::layout;
    double score = 0.0;
    std::string rationale;
};

class PhoneticLayout {
  public:
    virtual ~PhoneticLayout() = default;
    virtual std::string transliterate(const std::string& input) const = 0;
    virtual std::vector<Candidate> generate_candidates(const std::string& input) const = 0;
};

}  // namespace suborno::layout
