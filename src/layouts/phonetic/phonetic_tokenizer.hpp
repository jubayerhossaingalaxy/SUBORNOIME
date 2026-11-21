#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace suborno::layouts::phonetic::detail {

enum class TokenKind {
    vowel,
    consonant,
    nasal_n,
    nasal_ng,
    anusvara,
    visarga,
    cluster,
};

struct TokenDefinition {
    std::string_view input;
    TokenKind kind;
    std::string_view output;
    std::string_view dependent_output;
};

struct TokenMatch {
    const TokenDefinition* definition;
    std::size_t byte_length;
};

std::span<const TokenDefinition> token_registry() noexcept;
std::optional<TokenMatch> match_token(std::string_view input, std::size_t offset) noexcept;

}  // namespace suborno::layouts::phonetic::detail
