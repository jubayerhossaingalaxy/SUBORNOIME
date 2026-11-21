#include "layouts/phonetic/phonetic_tokenizer.hpp"

#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

#include "layouts/phonetic/phonetic_parser.hpp"

namespace {

using suborno::layouts::phonetic::detail::match_token;
using suborno::layouts::phonetic::detail::token_registry;
using suborno::layouts::phonetic::detail::TokenKind;
using suborno::layouts::phonetic::detail::TokenMatch;

struct TokenGolden {
    std::string_view input;
    std::string_view output;
};

constexpr std::array<TokenGolden, 91> kTokenGoldens{{
    {"chh", "\u099B"},
    {"ksh", "\u0995\u09CD\u09B7"},
    {"kkh", "\u0995\u09CD\u09B7"},
    {"jny", "\u099C\u09CD\u099E"},
    {"Th", "\u09A0"},
    {"Dh", "\u09A2"},
    {"Rh", "\u09A2\u09BC"},
    {"Ch", "\u099B"},
    {"Gh", "\u0998"},
    {"Jh", "\u099D"},
    {"Kh", "\u0996"},
    {"Ph", "\u09AB"},
    {"Bh", "\u09AD"},
    {"aa", "\u0986"},
    {"ii", "\u0988"},
    {"ee", "\u0988"},
    {"uu", "\u098A"},
    {"oo", "\u098A"},
    {"ri", "\u098B"},
    {"oi", "\u0990"},
    {"ai", "\u0990"},
    {"ou", "\u0994"},
    {"au", "\u0994"},
    {"kh", "\u0996"},
    {"gh", "\u0998"},
    {"ch", "\u099B"},
    {"jh", "\u099D"},
    {"ny", "\u099E"},
    {"th", "\u09A5"},
    {"dh", "\u09A7"},
    {"ph", "\u09AB"},
    {"bh", "\u09AD"},
    {"ng", "\u0982"},
    {"gy", "\u099C\u09CD\u099E"},
    {"jn", "\u099C\u09CD\u099E"},
    {"rh", "\u09B0\u09CD\u09B9"},
    {"sh", "\u09B6"},
    {"Sh", "\u09B7"},
    {"a", "\u0986"},
    {"ô", "\u0985"},
    {"O", "\u0985"},
    {"i", "\u0987"},
    {"u", "\u0989"},
    {"e", "\u098F"},
    {"o", "\u0993"},
    {"w", "\u0993"},
    {"W", "\u0993"},
    {"x", "\u0995\u09CD\u09B7"},
    {"k", "\u0995"},
    {"q", "\u0995"},
    {"g", "\u0997"},
    {"c", "\u099A"},
    {"j", "\u099C"},
    {"z", "\u099C"},
    {"T", "\u099F"},
    {"D", "\u09A1"},
    {"N", "\u09A3"},
    {"t", "\u09A4"},
    {"d", "\u09A6"},
    {"n", "\u09A8"},
    {"p", "\u09AA"},
    {"f", "\u09AB"},
    {"b", "\u09AC"},
    {"v", "\u09AD"},
    {"m", "\u09AE"},
    {"y", "\u09AF"},
    {"Y", "\u09AF\u09BC"},
    {"r", "\u09B0"},
    {"R", "\u09A1\u09BC"},
    {"l", "\u09B2"},
    {"s", "\u09B8"},
    {"h", "\u09B9"},
    {"M", "\u0982"},
    {"H", "\u0983"},
    {"A", "\u0986"},
    {"B", "\u09AC"},
    {"C", "\u099A"},
    {"E", "\u098F"},
    {"F", "\u09AB"},
    {"G", "\u0997"},
    {"I", "\u0987"},
    {"J", "\u099C"},
    {"K", "\u0995"},
    {"L", "\u09B2"},
    {"P", "\u09AA"},
    {"Q", "\u0995"},
    {"S", "\u09B8"},
    {"U", "\u0989"},
    {"V", "\u09AD"},
    {"X", "\u0995\u09CD\u09B7"},
    {"Z", "\u099C"},
}};

constexpr std::array<TokenGolden, 23> kDependentVowelGoldens{{
    {"a", "\u0995\u09BE"},  {"aa", "\u0995\u09BE"}, {"A", "\u0995\u09BE"},  {"ô", "\u0995"},
    {"O", "\u0995"},        {"i", "\u0995\u09BF"},  {"I", "\u0995\u09BF"},  {"ii", "\u0995\u09C0"},
    {"ee", "\u0995\u09C0"}, {"u", "\u0995\u09C1"},  {"U", "\u0995\u09C1"},  {"uu", "\u0995\u09C2"},
    {"oo", "\u0995\u09C2"}, {"ri", "\u0995\u09C3"}, {"e", "\u0995\u09C7"},  {"E", "\u0995\u09C7"},
    {"oi", "\u0995\u09C8"}, {"ai", "\u0995\u09C8"}, {"o", "\u0995\u09CB"},  {"w", "\u0995\u09CB"},
    {"W", "\u0995\u09CB"},  {"ou", "\u0995\u09CC"}, {"au", "\u0995\u09CC"},
}};

TokenMatch require_match(std::string_view input, std::size_t offset) {
    const auto match = match_token(input, offset);
    if (!match) {
        throw std::runtime_error("Expected a token match");
    }
    return *match;
}

void expect_token_golden(const TokenGolden& golden) {
    const auto match = match_token(golden.input, 0);
    if (!match.has_value()) {
        ADD_FAILURE() << "Expected token: " << golden.input;
        return;
    }

    const TokenMatch matched = *match;
    EXPECT_EQ(matched.definition->input, golden.input);
    EXPECT_EQ(matched.byte_length, golden.input.size());
    EXPECT_EQ(suborno::layouts::phonetic::detail::transliterate_primary(golden.input),
              golden.output)
        << golden.input;
}

bool has_token_golden(std::string_view input) {
    return std::ranges::any_of(kTokenGoldens, [input](const TokenGolden& golden) {
        return golden.input == input;
    });
}

}  // namespace

TEST(PhoneticTokenizerTest, RegistryIsImmutableAndContainsNormativeTokens) {
    const auto registry = token_registry();

    EXPECT_FALSE(registry.empty());
    EXPECT_TRUE(match_token("M", 0).has_value());
    EXPECT_TRUE(match_token("H", 0).has_value());
    EXPECT_TRUE(match_token("chh", 0).has_value());
    EXPECT_TRUE(match_token("ksh", 0).has_value());
    EXPECT_TRUE(match_token("kkh", 0).has_value());
    EXPECT_TRUE(match_token("x", 0).has_value());
}

TEST(PhoneticTokenizerTest, EveryNormativeTokenMatchesIndependentGoldenVectors) {
    for (const TokenGolden& golden : kTokenGoldens) {
        expect_token_golden(golden);
    }
}

TEST(PhoneticTokenizerTest, RegistryContainsExactlyTheNormativeInputTokens) {
    ASSERT_EQ(token_registry().size(), kTokenGoldens.size());
    std::unordered_set<std::string_view> unique_tokens;
    for (const auto& token : token_registry()) {
        EXPECT_TRUE(unique_tokens.insert(token.input).second) << token.input;
        EXPECT_TRUE(has_token_golden(token.input)) << token.input;
    }
}

TEST(PhoneticTokenizerTest, EveryNormativeVowelHasAnIndependentDependentForm) {
    for (const TokenGolden& golden : kDependentVowelGoldens) {
        const std::string input = "k" + std::string(golden.input);
        EXPECT_EQ(suborno::layouts::phonetic::detail::transliterate_primary(input), golden.output)
            << golden.input;
    }
}

TEST(PhoneticTokenizerTest, UsesLongestExactCaseSensitiveMatch) {
    const auto chh = require_match("chh", 0);
    EXPECT_EQ(chh.definition->input, "chh");
    EXPECT_EQ(chh.byte_length, 3U);
    EXPECT_EQ(chh.definition->output, "ছ");

    const auto retroflex = require_match("Th", 0);
    EXPECT_EQ(retroflex.definition->output, "ঠ");

    const auto dental = require_match("th", 0);
    EXPECT_EQ(dental.definition->output, "থ");

    const auto uppercase_first = require_match("TH", 0);
    EXPECT_EQ(uppercase_first.definition->input, "T");
    const auto uppercase_second = require_match("TH", 1);
    EXPECT_EQ(uppercase_second.definition->input, "H");
}

TEST(PhoneticTokenizerTest, KeepsSpecialSignsAndNasalsDistinct) {
    const auto anusvara = require_match("M", 0);
    const auto visarga = require_match("H", 0);
    const auto nasal_n = require_match("n", 0);
    const auto nasal_ng = require_match("ng", 0);

    EXPECT_EQ(anusvara.definition->kind, TokenKind::anusvara);
    EXPECT_EQ(anusvara.definition->output, "ং");
    EXPECT_EQ(visarga.definition->kind, TokenKind::visarga);
    EXPECT_EQ(visarga.definition->output, "ঃ");
    EXPECT_EQ(nasal_n.definition->kind, TokenKind::nasal_n);
    EXPECT_EQ(nasal_ng.definition->kind, TokenKind::nasal_ng);
}

TEST(PhoneticTokenizerTest, LowercaseRhIsAJoinerFreeConsonantCluster) {
    const auto rh = require_match("rh", 0);
    EXPECT_EQ(rh.definition->output, "র\u09CDহ");
    EXPECT_EQ(rh.definition->output.size(), std::string_view("র\u09CDহ").size());
}

TEST(PhoneticTokenizerTest, MatchesAtRequestedOffsetAndDoesNotConsumeUnknownInput) {
    const auto match = require_match("xTh", 1);
    EXPECT_EQ(match.definition->input, "Th");
    EXPECT_EQ(match.byte_length, 2U);
    const auto uppercase_first = require_match("TH", 0);
    EXPECT_EQ(uppercase_first.definition->input, "T");
    EXPECT_FALSE(match_token("t", 1).has_value());
    EXPECT_FALSE(match_token("x", 1).has_value());
    EXPECT_FALSE(match_token("t", std::string_view("t").size()).has_value());
}
