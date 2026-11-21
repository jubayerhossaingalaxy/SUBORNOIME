#include "phonetic_parser.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "phonetic_tokenizer.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts::phonetic::detail {
namespace {

enum class ParserState : std::uint8_t {
    boundary,
    consonant_run,
    after_vowel,
    explicit_halant,
};

enum class ControlAction : std::uint8_t {
    boundary,
    danda,
    double_danda,
    halant,
    halant_zwnj,
    halant_zwj,
    zwnj,
    zwj,
    nukta,
};

struct ControlDefinition {
    std::string_view token;
    ControlAction action;
};

using Alternative = std::vector<TokenDefinition>;

struct AmbiguitySite {
    std::size_t offset;
    std::size_t byte_length;
    std::vector<Alternative> alternatives;
};

struct CandidatePath {
    std::vector<std::size_t> choices;
    std::size_t penalty = 0;
    std::size_t first_alternative = 0;
};

struct ParseFlags {
    bool can_append_nukta = false;
    bool literal_passthrough = false;
};

struct ParseContext {
    std::string_view input;
    bool final_input;
    std::vector<AmbiguitySite>* discovered_sites;
    const std::vector<AmbiguitySite>* selected_sites;
    const std::vector<std::size_t>* choices;
    std::string result;
    ParserState state = ParserState::boundary;
    ParseFlags flags;
    std::size_t offset = 0;
    std::size_t site_index = 0;
    bool pending = false;
};

enum class ParseStep : std::uint8_t {
    not_handled,
    handled,
    stop,
};

constexpr std::array kControls{
    ControlDefinition{.token = ":boundary:", .action = ControlAction::boundary},
    ControlDefinition{.token = ":danda:", .action = ControlAction::danda},
    ControlDefinition{.token = ":ddanda:", .action = ControlAction::double_danda},
    ControlDefinition{.token = ":halant:", .action = ControlAction::halant},
    ControlDefinition{.token = ":halant-zwnj:", .action = ControlAction::halant_zwnj},
    ControlDefinition{.token = ":halant-zwj:", .action = ControlAction::halant_zwj},
    ControlDefinition{.token = ":zwnj:", .action = ControlAction::zwnj},
    ControlDefinition{.token = ":zwj:", .action = ControlAction::zwj},
    ControlDefinition{.token = ":nukta:", .action = ControlAction::nukta},
};

constexpr std::array<std::string_view, 10> kBengaliDigits{
    "০",
    "১",
    "২",
    "৩",
    "৪",
    "৫",
    "৬",
    "৭",
    "৮",
    "৯",
};
constexpr std::string_view kVirama = "\u09CD";
constexpr std::string_view kNukta = "\u09BC";
constexpr std::string_view kZwnj = "\u200C";
constexpr std::string_view kZwj = "\u200D";
constexpr std::string_view kDanda = "\u0964";
constexpr std::string_view kDoubleDanda = "\u0965";

std::size_t next_utf8_code_point_length(std::string_view input, std::size_t offset) {
    constexpr std::uint8_t kAsciiLimit = 0x80;
    constexpr std::uint8_t kThreeByteLimit = 0xE0;
    constexpr std::uint8_t kFourByteLimit = 0xF0;
    const auto lead = static_cast<std::uint8_t>(input[offset]);
    if (lead < kAsciiLimit) {
        return 1;
    }
    if (lead < kThreeByteLimit) {
        return 2;
    }
    if (lead < kFourByteLimit) {
        return 3;
    }
    return 4;
}

void append_consonant(std::string_view output,
                      std::string& result,
                      ParserState& state,
                      bool& can_append_nukta,
                      bool nukta_eligible = true) {
    if (state == ParserState::consonant_run) {
        result.append(kVirama);
    }
    result.append(output);
    state = ParserState::consonant_run;
    can_append_nukta = nukta_eligible;
}

void append_sign(std::string_view output,
                 std::string& result,
                 ParserState& state,
                 bool& can_append_nukta) {
    result.append(output);
    state = ParserState::after_vowel;
    can_append_nukta = false;
}

void append_vowel(const TokenDefinition& token,
                  std::string& result,
                  ParserState& state,
                  bool& can_append_nukta) {
    if (state == ParserState::consonant_run) {
        result.append(token.dependent_output);
    } else {
        result.append(token.output);
    }
    state = ParserState::after_vowel;
    can_append_nukta = false;
}

bool is_velar_token(std::string_view input, std::size_t offset) {
    const auto match = match_token(input, offset);
    if (!match.has_value()) {
        return false;
    }

    const std::string_view token = match->definition->input;
    return token == "k" || token == "kh" || token == "g" || token == "gh";
}

TokenDefinition token_definition(std::string_view input) {
    const auto match = match_token(input, 0);
    if (!match.has_value() || match->byte_length != input.size()) {
        throw std::logic_error("Phonetic alternate references an unknown token");
    }
    return *match->definition;
}

std::vector<Alternative> alternatives_for(const TokenDefinition& token) {
    if (token.input == "ch") {
        return {
            {token_definition("c"), token_definition("h")},
            {TokenDefinition{
                .input = "ch",
                .kind = TokenKind::consonant,
                .output = "চ",
                .dependent_output = {},
            }},
        };
    }
    if (token.input == "sh") {
        return {
            {TokenDefinition{
                .input = "sh",
                .kind = TokenKind::consonant,
                .output = "ষ",
                .dependent_output = {},
            }},
            {token_definition("s"), token_definition("h")},
        };
    }
    if (token.input == "chh") {
        return {{token_definition("ch"), token_definition("h")}};
    }
    if (token.input == "ksh") {
        return {{token_definition("k"), token_definition("sh")}};
    }
    if (token.input == "kkh") {
        return {{token_definition("k"), token_definition("kh")}};
    }
    if (token.input == "ai") {
        return {{token_definition("a"), token_definition("i")}};
    }
    if (token.input == "oi") {
        return {{token_definition("o"), token_definition("i")}};
    }
    if (token.input == "au") {
        return {{token_definition("a"), token_definition("u")}};
    }
    if (token.input == "ou") {
        return {{token_definition("o"), token_definition("u")}};
    }
    return {};
}

const ControlDefinition* find_control(std::string_view token) {
    for (const ControlDefinition& control : kControls) {
        if (control.token == token) {
            return &control;
        }
    }
    return nullptr;
}

void process_token(const TokenDefinition& token,
                   std::string_view input,
                   std::size_t next_input_offset,
                   std::string& result,
                   ParserState& state,
                   bool& can_append_nukta) {
    switch (token.kind) {
    case TokenKind::vowel:
        append_vowel(token, result, state, can_append_nukta);
        return;
    case TokenKind::consonant:
    case TokenKind::cluster:
        append_consonant(token.output,
                         result,
                         state,
                         can_append_nukta,
                         token.input != "R" && token.input != "Rh" && token.input != "Y");
        return;
    case TokenKind::nasal_ng:
        if (const auto next = match_token(input, next_input_offset);
            next.has_value() && next->definition->kind == TokenKind::vowel) {
            append_consonant(token.dependent_output, result, state, can_append_nukta);
        } else {
            append_sign(token.output, result, state, can_append_nukta);
        }
        return;
    case TokenKind::nasal_n:
        if (is_velar_token(input, next_input_offset)) {
            append_sign(token.dependent_output, result, state, can_append_nukta);
        } else {
            append_consonant(token.output, result, state, can_append_nukta);
        }
        return;
    case TokenKind::anusvara:
    case TokenKind::visarga:
        append_sign(token.output, result, state, can_append_nukta);
        return;
    }
}

void append_literal(std::string_view literal,
                    std::string& result,
                    ParserState& state,
                    ParseFlags& flags) {
    result.append(literal);
    state = ParserState::boundary;
    flags.can_append_nukta = false;
    flags.literal_passthrough = true;
}

void process_control(const ControlDefinition& control,
                     std::string_view token,
                     std::string& result,
                     ParserState& state,
                     ParseFlags& flags) {
    switch (control.action) {
    case ControlAction::boundary:
        state = ParserState::boundary;
        flags.can_append_nukta = false;
        return;
    case ControlAction::danda:
        result.append(kDanda);
        state = ParserState::boundary;
        flags.can_append_nukta = false;
        return;
    case ControlAction::double_danda:
        result.append(kDoubleDanda);
        state = ParserState::boundary;
        flags.can_append_nukta = false;
        return;
    case ControlAction::halant:
        if (state == ParserState::consonant_run) {
            result.append(kVirama);
            state = ParserState::explicit_halant;
            flags.can_append_nukta = false;
        } else {
            append_literal(token, result, state, flags);
        }
        return;
    case ControlAction::halant_zwnj:
    case ControlAction::halant_zwj:
        if (state == ParserState::consonant_run) {
            result.append(kVirama);
            result.append(control.action == ControlAction::halant_zwnj ? kZwnj : kZwj);
            state = ParserState::boundary;
            flags.can_append_nukta = false;
        } else {
            append_literal(token, result, state, flags);
        }
        return;
    case ControlAction::zwnj:
    case ControlAction::zwj:
        if (state == ParserState::explicit_halant) {
            result.append(control.action == ControlAction::zwnj ? kZwnj : kZwj);
            state = ParserState::boundary;
            flags.can_append_nukta = false;
        } else {
            append_literal(token, result, state, flags);
        }
        return;
    case ControlAction::nukta:
        if (state == ParserState::consonant_run && flags.can_append_nukta) {
            result.append(kNukta);
            flags.can_append_nukta = false;
        } else {
            append_literal(token, result, state, flags);
        }
        return;
    }
}

ParseStep process_escape(ParseContext& context) {
    if (context.input[context.offset] != '\\') {
        return ParseStep::not_handled;
    }

    if (context.offset + 1 == context.input.size()) {
        if (!context.final_input) {
            context.pending = true;
        } else {
            context.result.push_back('\\');
            context.flags.literal_passthrough = true;
        }
        return ParseStep::stop;
    }

    const auto graphemes =
        suborno::unicode::split_graphemes(std::string(context.input.substr(context.offset + 1)));
    const std::string& escaped = graphemes.front();
    context.result.append(escaped);
    context.offset += 1 + escaped.size();
    context.state = ParserState::boundary;
    context.flags.can_append_nukta = false;
    context.flags.literal_passthrough = true;
    return ParseStep::handled;
}

ParseStep process_control_input(ParseContext& context) {
    if (context.input[context.offset] != ':') {
        return ParseStep::not_handled;
    }

    const std::size_t close = context.input.find(':', context.offset + 1);
    if (close == std::string_view::npos) {
        if (!context.final_input) {
            context.pending = true;
        } else {
            append_literal(
                context.input.substr(context.offset), context.result, context.state, context.flags);
        }
        return ParseStep::stop;
    }

    const std::string_view token = context.input.substr(context.offset, close - context.offset + 1);
    if (const ControlDefinition* control = find_control(token)) {
        process_control(*control, token, context.result, context.state, context.flags);
    } else {
        append_literal(token, context.result, context.state, context.flags);
    }
    context.offset = close + 1;
    return ParseStep::handled;
}

bool process_digit(ParseContext& context) {
    const auto byte = static_cast<unsigned char>(context.input[context.offset]);
    if (byte < static_cast<unsigned char>('0') || byte > static_cast<unsigned char>('9')) {
        return false;
    }

    context.result.append(kBengaliDigits[byte - static_cast<unsigned char>('0')]);
    context.state = ParserState::boundary;
    context.flags.can_append_nukta = false;
    ++context.offset;
    return true;
}

bool process_matched_token(ParseContext& context) {
    const auto match = match_token(context.input, context.offset);
    if (!match) {
        return false;
    }

    const TokenDefinition& token = *match->definition;
    if (context.state == ParserState::explicit_halant
        && (token.kind == TokenKind::anusvara || token.kind == TokenKind::visarga)) {
        append_literal(token.input, context.result, context.state, context.flags);
        context.offset += match->byte_length;
        return true;
    }

    if (context.selected_sites != nullptr && context.site_index < context.selected_sites->size()
        && (*context.selected_sites)[context.site_index].offset == context.offset) {
        const AmbiguitySite& site = (*context.selected_sites)[context.site_index];
        const std::size_t choice = (*context.choices)[context.site_index];
        ++context.site_index;
        if (choice > 0) {
            for (const TokenDefinition& alternate_token : site.alternatives[choice - 1]) {
                process_token(alternate_token,
                              context.input,
                              context.offset + site.byte_length,
                              context.result,
                              context.state,
                              context.flags.can_append_nukta);
            }
            context.offset += site.byte_length;
            return true;
        }
    }

    if (context.discovered_sites != nullptr) {
        auto alternatives = alternatives_for(token);
        if (!alternatives.empty()) {
            context.discovered_sites->push_back({
                .offset = context.offset,
                .byte_length = match->byte_length,
                .alternatives = std::move(alternatives),
            });
        }
    }

    process_token(token,
                  context.input,
                  context.offset + match->byte_length,
                  context.result,
                  context.state,
                  context.flags.can_append_nukta);
    context.offset += match->byte_length;
    return true;
}

}  // namespace

ParseResult parse_internal(std::string_view input,
                           bool final_input,
                           std::vector<AmbiguitySite>* discovered_sites,
                           const std::vector<AmbiguitySite>* selected_sites,
                           const std::vector<std::size_t>* choices) {
    if (!suborno::unicode::is_valid_utf8(input)) {
        throw std::invalid_argument("Input is not valid UTF-8");
    }

    ParseContext context{
        .input = input,
        .final_input = final_input,
        .discovered_sites = discovered_sites,
        .selected_sites = selected_sites,
        .choices = choices,
        .result = {},
        .state = ParserState::boundary,
        .flags = {},
        .offset = 0,
        .site_index = 0,
        .pending = false,
    };
    context.result.reserve(input.size());
    while (context.offset < input.size()) {
        const ParseStep escape_step = process_escape(context);
        if (escape_step == ParseStep::stop) {
            break;
        }
        if (escape_step == ParseStep::handled) {
            continue;
        }
        const ParseStep control_step = process_control_input(context);
        if (control_step == ParseStep::stop) {
            break;
        }
        if (control_step == ParseStep::handled || process_digit(context)
            || process_matched_token(context)) {
            continue;
        }

        const std::size_t length = next_utf8_code_point_length(input, context.offset);
        context.result.append(input.substr(context.offset, length));
        context.state = ParserState::boundary;
        context.flags.can_append_nukta = false;
        context.flags.literal_passthrough = true;
        context.offset += length;
    }

    return {
        .text = suborno::unicode::normalize_nfc(context.result),
        .pending = context.pending,
        .literal_passthrough = context.flags.literal_passthrough,
    };
}

ParseResult parse_primary(std::string_view input, bool final_input) {
    return parse_internal(input, final_input, nullptr, nullptr, nullptr);
}

std::string transliterate_primary(std::string_view input) {
    return parse_primary(input, true).text;
}

std::vector<suborno::layout::Candidate> generate_candidates(std::string_view input) {
    constexpr std::size_t kMaxCandidates = 16;

    std::vector<AmbiguitySite> sites;
    const ParseResult primary = parse_internal(input, true, &sites, nullptr, nullptr);
    std::vector<CandidatePath> paths(1);

    for (const AmbiguitySite& site : sites) {
        std::vector<CandidatePath> expanded;
        expanded.reserve(paths.size() * (site.alternatives.size() + 1));
        for (const CandidatePath& path : paths) {
            for (std::size_t choice = 0; choice <= site.alternatives.size(); ++choice) {
                CandidatePath candidate = path;
                candidate.choices.push_back(choice);
                candidate.penalty += choice;
                if (choice > 0 && candidate.first_alternative == 0) {
                    candidate.first_alternative = candidate.choices.size();
                }
                expanded.push_back(std::move(candidate));
            }
        }

        std::ranges::sort(expanded, [](const CandidatePath& left, const CandidatePath& right) {
            return std::tie(left.penalty, left.first_alternative, left.choices)
                   < std::tie(right.penalty, right.first_alternative, right.choices);
        });
        if (expanded.size() > kMaxCandidates) {
            expanded.resize(kMaxCandidates);
        }
        paths = std::move(expanded);
    }

    std::vector<suborno::layout::Candidate> candidates;
    candidates.reserve(paths.size());
    std::unordered_set<std::string> seen;
    seen.reserve(paths.size());
    for (const CandidatePath& path : paths) {
        const ParseResult parsed = parse_internal(input, true, nullptr, &sites, &path.choices);
        if (!seen.insert(parsed.text).second) {
            continue;
        }
        const std::size_t rank = candidates.size();
        candidates.push_back({
            .text = parsed.text,
            .normalized_text = parsed.text,
            .stable_id = "layout:" + parsed.text,
            .source = suborno::layout::CandidateSource::layout,
            .score = 1.0 / static_cast<double>(rank + 1),
            .rationale = rank == 0 ? "primary phonetic output" : "phonetic alternative combination",
        });
    }

    if (candidates.empty()) {
        candidates.push_back({
            .text = primary.text,
            .normalized_text = primary.text,
            .stable_id = "layout:" + primary.text,
            .source = suborno::layout::CandidateSource::layout,
            .score = 1.0,
            .rationale = "primary phonetic output",
        });
    }
    return candidates;
}

}  // namespace suborno::layouts::phonetic::detail
