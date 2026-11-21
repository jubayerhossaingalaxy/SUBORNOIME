#include "suborno/layouts/phonetic/rule_based_phonetic_layout.hpp"

#include <array>
#include <stdexcept>
#include <string_view>

#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts::phonetic {
namespace {

enum class CompositionState {
    boundary,
    consonant_run,
    after_vowel,
    explicit_halant,
};

struct ControlToken {
    std::string_view input;
    std::string_view output;
    CompositionState next_state;
};

constexpr std::string_view kVirama = "\u09CD";

constexpr std::array<std::string_view, 10> kBanglaDigits{
    "০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯"};

constexpr std::array<ControlToken, 7> kControls{{
    {",..", "\u09CD\u200D", CompositionState::boundary},
    {",.", "\u09CD\u200C", CompositionState::boundary},
    {",,", "\u09CD", CompositionState::explicit_halant},
    {"./", "\u09BC", CompositionState::consonant_run},
    {"::", "\u0983", CompositionState::after_vowel},
    {"^", "\u0981", CompositionState::after_vowel},
    {".", "\u0964", CompositionState::boundary},
}};

struct TokenMatch {
    const PhoneticTokenDefinition* definition = nullptr;
    std::size_t length = 0;
};

TokenMatch match_token(std::span<const PhoneticTokenDefinition> tokens,
                       std::string_view input,
                       std::size_t offset) {
    TokenMatch best;
    for (const PhoneticTokenDefinition& token : tokens) {
        if (token.input.size() <= input.size() - offset
            && token.input.size() > best.length
            && input.compare(offset, token.input.size(), token.input) == 0) {
            best = {&token, token.input.size()};
        }
    }
    return best;
}

const ControlToken* match_control(std::string_view input, std::size_t offset) {
    for (const ControlToken& control : kControls) {
        if (control.input.size() <= input.size() - offset
            && input.compare(offset, control.input.size(), control.input) == 0) {
            return &control;
        }
    }
    return nullptr;
}

std::size_t code_point_length(std::string_view input, std::size_t offset) {
    const auto lead = static_cast<unsigned char>(input[offset]);
    if (lead < 0x80) {
        return 1;
    }
    if (lead < 0xE0) {
        return 2;
    }
    if (lead < 0xF0) {
        return 3;
    }
    return 4;
}

void append_consonant(const PhoneticTokenDefinition& token,
                      std::string& output,
                      CompositionState& state,
                      bool& nukta_eligible) {
    if (state == CompositionState::consonant_run) {
        output.append(kVirama);
    }
    output.append(token.dependent_output.empty() ? token.output : token.dependent_output);
    state = CompositionState::consonant_run;
    nukta_eligible = token.nukta_eligible;
}

void append_token(const PhoneticTokenDefinition& token,
                  std::string& output,
                  CompositionState& state,
                  bool& nukta_eligible) {
    switch (token.kind) {
    case PhoneticTokenKind::consonant:
        append_consonant(token, output, state, nukta_eligible);
        return;
    case PhoneticTokenKind::vowel:
        if (state == CompositionState::consonant_run) {
            output.append(token.dependent_output);
        } else {
            output.append(token.output);
        }
        state = CompositionState::after_vowel;
        nukta_eligible = false;
        return;
    case PhoneticTokenKind::sign:
        output.append(token.output);
        state = CompositionState::after_vowel;
        nukta_eligible = false;
        return;
    case PhoneticTokenKind::halant_prefix:
        output.append(token.output);
        state = CompositionState::explicit_halant;
        nukta_eligible = false;
        return;
    }
}

void append_escaped_grapheme(std::string_view input,
                             std::size_t& offset,
                             std::string& output,
                             CompositionState& state,
                             bool& nukta_eligible) {
    if (offset + 1 >= input.size()) {
        output.push_back('\\');
        ++offset;
        state = CompositionState::boundary;
        nukta_eligible = false;
        return;
    }

    const auto graphemes =
        suborno::unicode::split_graphemes(std::string(input.substr(offset + 1)));
    if (graphemes.empty()) {
        output.push_back('\\');
        ++offset;
    } else {
        output.append(graphemes.front());
        offset += 1 + graphemes.front().size();
    }
    state = CompositionState::boundary;
    nukta_eligible = false;
}

}  // namespace

RuleBasedPhoneticLayout::RuleBasedPhoneticLayout(
    std::span<const PhoneticTokenDefinition> tokens,
    bool accent_key_enabled,
    std::string_view profile_id)
    : tokens_(tokens),
      accent_key_enabled_(accent_key_enabled),
      profile_id_(profile_id) {
    if (tokens_.empty() || profile_id_.empty()) {
        throw std::invalid_argument("A phonetic layout requires tokens and a profile identifier");
    }

    for (std::size_t index = 0; index < tokens_.size(); ++index) {
        if (tokens_[index].input.empty() || tokens_[index].output.empty()) {
            throw std::invalid_argument("Phonetic tokens must have input and output text");
        }
        for (std::size_t other = index + 1; other < tokens_.size(); ++other) {
            if (tokens_[index].input == tokens_[other].input) {
                throw std::invalid_argument("Phonetic token inputs must be unique");
            }
        }
    }
}

std::string RuleBasedPhoneticLayout::transliterate(const std::string& input) const {
    if (!suborno::unicode::is_valid_utf8(input)) {
        throw std::invalid_argument("Input is not valid UTF-8");
    }

    std::string output;
    output.reserve(input.size());
    std::size_t offset = 0;
    CompositionState state = CompositionState::boundary;
    bool nukta_eligible = false;
    bool force_independent_vowel = false;

    while (offset < input.size()) {
        if (input[offset] == '\\') {
            append_escaped_grapheme(input, offset, output, state, nukta_eligible);
            force_independent_vowel = false;
            continue;
        }

        if (accent_key_enabled_ && input[offset] == '`') {
            if (state == CompositionState::consonant_run) {
                state = CompositionState::boundary;
            }
            force_independent_vowel = true;
            ++offset;
            continue;
        }

        if (const ControlToken* control = match_control(input, offset);
            control != nullptr) {
            const bool has_consonant = state == CompositionState::consonant_run;
            const bool valid_nukta =
                control->input == "./" && has_consonant && nukta_eligible;
            const bool valid_halant = control->input == ",," && has_consonant;
            const bool valid_joiner =
                (control->input == ",." || control->input == ",..") && has_consonant;
            const bool valid_sign =
                control->input == "::" || control->input == "^" || control->input == ".";
            if (valid_nukta || valid_halant || valid_joiner || valid_sign) {
                output.append(control->output);
                state = control->next_state;
                nukta_eligible = false;
            } else {
                output.append(control->input);
                state = CompositionState::boundary;
                nukta_eligible = false;
            }
            offset += control->input.size();
            force_independent_vowel = false;
            continue;
        }

        const auto byte = static_cast<unsigned char>(input[offset]);
        if (byte >= static_cast<unsigned char>('0')
            && byte <= static_cast<unsigned char>('9')) {
            output.append(kBanglaDigits[byte - static_cast<unsigned char>('0')]);
            state = CompositionState::boundary;
            nukta_eligible = false;
            force_independent_vowel = false;
            ++offset;
            continue;
        }

        const TokenMatch match = match_token(tokens_, input, offset);
        if (match.definition != nullptr) {
            bool consumed_accent = false;
            if (match.definition->kind == PhoneticTokenKind::vowel && accent_key_enabled_
                && offset + match.length < input.size()
                && input[offset + match.length] == '`') {
                output.append(match.definition->dependent_output);
                state = CompositionState::after_vowel;
                nukta_eligible = false;
                consumed_accent = true;
            } else if (match.definition->kind == PhoneticTokenKind::vowel
                       && force_independent_vowel) {
                output.append(match.definition->output);
                state = CompositionState::after_vowel;
                nukta_eligible = false;
            } else if (state == CompositionState::explicit_halant
                       && match.definition->kind == PhoneticTokenKind::consonant) {
                output.append(match.definition->output);
                state = CompositionState::consonant_run;
                nukta_eligible = match.definition->nukta_eligible;
            } else {
                append_token(*match.definition, output, state, nukta_eligible);
            }
            force_independent_vowel = false;
            offset += match.length + (consumed_accent ? 1U : 0U);
            continue;
        }

        const std::size_t length = code_point_length(input, offset);
        output.append(input, offset, length);
        offset += length;
        state = CompositionState::boundary;
        nukta_eligible = false;
        force_independent_vowel = false;
    }

    return suborno::unicode::normalize_nfc(output);
}

std::vector<suborno::layout::Candidate> RuleBasedPhoneticLayout::generate_candidates(
    const std::string& input) const {
    const std::string output = transliterate(input);
    return {{
        .text = output,
        .normalized_text = output,
        .stable_id = std::string(profile_id_) + ":" + output,
        .source = suborno::layout::CandidateSource::layout,
        .score = 1.0,
        .rationale = "phonetic rule output",
    }};
}

}  // namespace suborno::layouts::phonetic
