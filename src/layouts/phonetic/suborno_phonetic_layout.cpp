#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

#include <string>
#include <vector>

#include "phonetic_parser.hpp"

namespace suborno::layouts {
std::string SubornoPhoneticLayout::transliterate(const std::string& input) const {
    return suborno::layouts::phonetic::detail::transliterate_primary(input);
}

std::vector<suborno::layout::Candidate> SubornoPhoneticLayout::generate_candidates(
    const std::string& input) const {
    return suborno::layouts::phonetic::detail::generate_candidates(input);
}

}  // namespace suborno::layouts
