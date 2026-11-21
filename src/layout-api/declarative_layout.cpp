#include "suborno/layout-api/declarative_layout.hpp"

#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <utility>

#include "suborno/unicode/bengali.hpp"

namespace suborno::layout {

DeclarativeLayout::DeclarativeLayout(std::string id, std::filesystem::path mapping_file)
    : id_(std::move(id)),
      mapping_file_(std::move(mapping_file)) {}

bool DeclarativeLayout::load() {
    std::ifstream input(mapping_file_);
    if (!input.is_open()) {
        return false;
    }

    std::unordered_map<std::string, std::string> loaded_mapping;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const std::size_t separator = line.find('\t');
        if (separator == std::string::npos || separator == 0 || separator + 1 >= line.size()) {
            return false;
        }
        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (!suborno::unicode::is_valid_utf8(key) || !suborno::unicode::is_valid_utf8(value)
            || !loaded_mapping.emplace(key, value).second) {
            return false;
        }
    }
    if (input.bad()) {
        return false;
    }
    mapping_ = std::move(loaded_mapping);
    return true;
}

const std::string& DeclarativeLayout::id() const noexcept {
    return id_;
}

std::string DeclarativeLayout::transliterate(const std::string& input) const {
    std::string output;
    std::size_t offset = 0;
    while (offset < input.size()) {
        std::string matched;
        std::size_t matched_length = 0;
        for (const auto& [key, value] : mapping_) {
            if (input.compare(offset, key.size(), key) == 0 && key.size() > matched_length) {
                matched = value;
                matched_length = key.size();
            }
        }
        if (matched_length == 0) {
            const unsigned char lead = static_cast<unsigned char>(input[offset]);
            std::size_t length = 1;
            if ((lead & 0xE0U) == 0xC0U) {
                length = 2;
            } else if ((lead & 0xF0U) == 0xE0U) {
                length = 3;
            } else if ((lead & 0xF8U) == 0xF0U) {
                length = 4;
            }
            output.append(input, offset, length);
            offset += length;
        } else {
            output.append(matched);
            offset += matched_length;
        }
    }
    return suborno::unicode::normalize_nfc(output);
}

std::vector<Candidate> DeclarativeLayout::generate_candidates(const std::string& input) const {
    const std::string output = transliterate(input);
    return {{output,
             output,
             "layout:" + id_ + ":" + output,
             CandidateSource::layout,
             1.0,
             "declarative layout"}};
}

}  // namespace suborno::layout
