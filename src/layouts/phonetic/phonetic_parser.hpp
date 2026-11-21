#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "suborno/layout-api/phonetic_layout.hpp"

namespace suborno::layouts::phonetic::detail {

struct ParseResult {
    std::string text;
    bool pending = false;
    bool literal_passthrough = false;
};

ParseResult parse_primary(std::string_view input, bool final_input);
std::string transliterate_primary(std::string_view input);
std::vector<suborno::layout::Candidate> generate_candidates(std::string_view input);

}  // namespace suborno::layouts::phonetic::detail
