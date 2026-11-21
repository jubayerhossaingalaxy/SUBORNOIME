#pragma once

#include <string>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::spell {

struct SpellingResult {
    bool is_correct = false;
    std::vector<suborno::layout::Candidate> suggestions;
};

class SpellChecker {
  public:
    explicit SpellChecker(const dictionary::Dictionary& dictionary);

    SpellingResult check(const std::string& word) const;

    std::vector<std::string> generate_edits(const std::string& word) const;
    double edit_distance(const std::string& a, const std::string& b) const;

  private:
    const dictionary::Dictionary& dictionary_;
};

}  // namespace suborno::spell
