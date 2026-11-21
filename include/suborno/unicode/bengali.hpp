#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace suborno::unicode {

struct BengaliCharacterInfo {
    std::string character;
    std::string name;
    std::string category;
    std::string romanization;
    std::string normalized_form;
};

bool is_valid_utf8(std::string_view text) noexcept;
std::size_t count_utf8_code_points(std::string_view text);
std::string normalize_nfc(std::string_view text);
std::string normalize_nfkc(const std::string& text);
std::vector<BengaliCharacterInfo> supported_bengali_inventory();
bool is_bengali_codepoint(unsigned int code_point);
std::vector<std::string> split_graphemes(const std::string& text);

}  // namespace suborno::unicode
