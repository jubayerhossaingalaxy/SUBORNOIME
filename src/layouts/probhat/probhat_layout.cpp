#include "suborno/layouts/probhat/probhat_layout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

struct ProbhatKeyMapping {
    std::uint8_t scan_code;
    std::string_view unshifted;
    std::string_view shifted;
    std::string_view unshifted_dependent{};
    std::string_view shifted_dependent{};
};

// Independently authored mapping from the published Probhat key chart.
// Scan codes use the IBM PC/AT set-1 positions exposed by Windows key events.
constexpr ProbhatKeyMapping kProbhatMappings[] = {
    {0x02, "১", "!"},
    {0x03, "২", "@"},
    {0x04, "৩", "#"},
    {0x05, "৪", "৳"},
    {0x06, "৫", "%"},
    {0x07, "৬", "^"},
    {0x08, "৭", "ঞ"},
    {0x09, "৮", "ৎ"},
    {0x0A, "৯", "("},
    {0x0B, "০", ")"},
    {0x0C, "-", "_"},
    {0x0D, "=", "+"},
    {0x10, "দ", "ধ", {}, {}},
    {0x11, "উ", "ঊ", "ূ", "ূ"},
    {0x12, "ই", "ঈ", "ী", "ী"},
    {0x13, "র", "ড়", {}, {}},
    {0x14, "ট", "ঠ", {}, {}},
    {0x15, "এ", "ঐ", "ে", "ৈ"},
    {0x16, "উ", "উ", "ু", "ু"},
    {0x17, "ই", "ই", "ি", "ি"},
    {0x18, "ও", "ঔ", "ো", "ৌ"},
    {0x19, "প", "ফ", {}, {}},
    {0x1A, "এ", "ঐ", "ে", "ৈ"},
    {0x1B, "ও", "ঔ", "ো", "ৌ"},
    {0x1E, "আ", "অ", "া", "অ"},
    {0x1F, "স", "ষ"},
    {0x20, "ড", "ঢ"},
    {0x21, "ত", "থ"},
    {0x22, "গ", "ঘ"},
    {0x23, "হ", "ঃ"},
    {0x24, "জ", "ঝ"},
    {0x25, "ক", "খ"},
    {0x26, "ল", "ং"},
    {0x27, ";", ":"},
    {0x28, "'", R"(")"},
    {0x29, "\u200D", "~"},
    {0x2B, "\\", "|"},
    {0x2C, "য়", "য"},
    {0x2D, "শ", "ঢ়"},
    {0x2E, "চ", "ছ"},
    {0x2F, "আ", "ঋ"},
    {0x30, "ব", "ভ"},
    {0x31, "ন", "ণ"},
    {0x32, "ম", "ঙ"},
    {0x33, ",", "ৃ"},
    {0x34, "।", "ঁ"},
    {0x35, "্", "?"},
    {0x39, " ", " "},
};

const ProbhatKeyMapping* find_mapping(std::uint8_t scan_code) noexcept {
    for (const ProbhatKeyMapping& mapping : kProbhatMappings) {
        if (mapping.scan_code == scan_code) {
            return &mapping;
        }
    }
    return nullptr;
}

bool ascii_key_to_event(char value, layout::KeyEvent& event) noexcept {
    event = {};
    event.key_down = true;

    constexpr std::array<std::uint8_t, 26> kLetterScanCodes{
        0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
        0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C};

    if (value >= 'a' && value <= 'z') {
        event.scan_code = kLetterScanCodes[static_cast<std::size_t>(value - 'a')];
        return true;
    }
    if (value >= 'A' && value <= 'Z') {
        event.scan_code = kLetterScanCodes[static_cast<std::size_t>(value - 'A')];
        event.modifiers.shift = true;
        return true;
    }
    if (value >= '1' && value <= '9') {
        event.scan_code = static_cast<std::uint8_t>(0x02 + (value - '1'));
        return true;
    }
    if (value == '0') {
        event.scan_code = 0x0B;
        return true;
    }

    switch (static_cast<unsigned char>(value)) {
    case 45:
        event.scan_code = 0x0C;
        return true;  // -
    case 95:
        event.scan_code = 0x0C;
        event.modifiers.shift = true;
        return true;  // _
    case 61:
        event.scan_code = 0x0D;
        return true;  // =
    case 43:
        event.scan_code = 0x0D;
        event.modifiers.shift = true;
        return true;  // +
    case 91:
        event.scan_code = 0x1A;
        return true;  // [
    case 123:
        event.scan_code = 0x1A;
        event.modifiers.shift = true;
        return true;  // {
    case 93:
        event.scan_code = 0x1B;
        return true;  // ]
    case 125:
        event.scan_code = 0x1B;
        event.modifiers.shift = true;
        return true;  // }
    case 59:
        event.scan_code = 0x27;
        return true;  // ;
    case 58:
        event.scan_code = 0x27;
        event.modifiers.shift = true;
        return true;  // :
    case 39:
        event.scan_code = 0x28;
        return true;  // '
    case 34:
        event.scan_code = 0x28;
        event.modifiers.shift = true;
        return true;  // "
    case 96:
        event.scan_code = 0x29;
        return true;  // grave
    case 126:
        event.scan_code = 0x29;
        event.modifiers.shift = true;
        return true;  // ~
    case 92:
        event.scan_code = 0x2B;
        return true;  // backslash
    case 124:
        event.scan_code = 0x2B;
        event.modifiers.shift = true;
        return true;  // |
    case 44:
        event.scan_code = 0x33;
        return true;  // ,
    case 60:
        event.scan_code = 0x33;
        event.modifiers.shift = true;
        return true;  // <
    case 46:
        event.scan_code = 0x34;
        return true;  // .
    case 62:
        event.scan_code = 0x34;
        event.modifiers.shift = true;
        return true;  // >
    case 47:
        event.scan_code = 0x35;
        return true;  // /
    case 63:
        event.scan_code = 0x35;
        event.modifiers.shift = true;
        return true;  // ?
    case 32:
        event.scan_code = 0x39;
        return true;  // space
    default:
        return false;
    }
}

bool ends_with_bengali_consonant(std::string_view text) {
    const auto graphemes = suborno::unicode::split_graphemes(std::string(text));
    if (graphemes.empty()) {
        return false;
    }

    const std::string& last_grapheme = graphemes.back();
    if (last_grapheme.size() < 3) {
        return false;
    }
    const auto* bytes = reinterpret_cast<const unsigned char*>(last_grapheme.data());
    std::size_t start = last_grapheme.size() - 1;
    while (start > 0 && (bytes[start] & 0xC0U) == 0x80U) {
        --start;
    }
    const unsigned char lead = bytes[start];
    char32_t code_point = 0;
    if (lead < 0x80) {
        code_point = lead;
    } else if ((lead & 0xE0U) == 0xC0U && last_grapheme.size() - start >= 2) {
        code_point = static_cast<char32_t>(((lead & 0x1FU) << 6U) | (bytes[start + 1] & 0x3FU));
    } else if ((lead & 0xF0U) == 0xE0U && last_grapheme.size() - start >= 3) {
        code_point =
            static_cast<char32_t>(((lead & 0x0FU) << 12U) | ((bytes[start + 1] & 0x3FU) << 6U)
                                  | (bytes[start + 2] & 0x3FU));
    } else {
        return false;
    }
    return code_point >= 0x0995 && code_point <= 0x09B9;
}

std::string output_for_event(const layout::KeyEvent& event, bool after_consonant) {
    if (!event.key_down || event.modifiers.control || event.modifiers.alt
        || event.modifiers.windows) {
        return {};
    }

    const auto physical_key = static_cast<std::uint8_t>(event.key);
    const auto first_numpad = static_cast<std::uint8_t>(layout::PhysicalKey::numpad_0);
    const auto last_numpad = static_cast<std::uint8_t>(layout::PhysicalKey::numpad_9);
    if (physical_key >= first_numpad && physical_key <= last_numpad) {
        if (!event.modifiers.num_lock) {
            return {};
        }
        constexpr std::string_view kBengaliDigits[] = {
            "০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯"};
        return std::string(kBengaliDigits[physical_key - first_numpad]);
    }

    if (event.scan_code > std::numeric_limits<std::uint8_t>::max()) {
        return {};
    }
    const ProbhatKeyMapping* mapping = find_mapping(static_cast<std::uint8_t>(event.scan_code));
    if (mapping == nullptr) {
        return {};
    }
    if (after_consonant && !mapping->unshifted_dependent.empty()) {
        return std::string(event.modifiers.shift ? mapping->shifted_dependent
                                                 : mapping->unshifted_dependent);
    }
    return std::string(event.modifiers.shift ? mapping->shifted : mapping->unshifted);
}

std::string transliterate_probhat(std::string_view input) {
    if (!suborno::unicode::is_valid_utf8(input)) {
        return {};
    }

    std::string output;
    output.reserve(input.size() * 3U);
    for (const char value : input) {
        layout::KeyEvent event;
        if (ascii_key_to_event(value, event)) {
            output.append(output_for_event(event, ends_with_bengali_consonant(output)));
        } else {
            output.push_back(value);
        }
    }
    return suborno::unicode::normalize_nfc(output);
}

class ProbhatInputSession final : public layout::InputSession {
  public:
    layout::InputStatus append_utf8(std::string_view input) override {
        if (!suborno::unicode::is_valid_utf8(input)) {
            return layout::InputStatus::invalid_utf8;
        }
        if (input.empty()) {
            return layout::InputStatus::empty;
        }
        std::string updated = composition_;
        for (const char value : input) {
            layout::KeyEvent event;
            if (ascii_key_to_event(value, event)) {
                updated.append(output_for_event(event, ends_with_bengali_consonant(updated)));
            } else {
                updated.push_back(value);
            }
        }
        return append_output(updated);
    }

    layout::InputStatus append_key(const layout::KeyEvent& event) override {
        if (!event.key_down || event.modifiers.control || event.modifiers.alt
            || event.modifiers.windows) {
            return layout::InputStatus::empty;
        }
        const std::size_t repeat_count = event.repeat_count == 0 ? 1 : event.repeat_count;
        if (repeat_count > layout::InputSession::kMaxInputCodePoints) {
            return layout::InputStatus::input_limit;
        }

        std::string updated = composition_;
        for (std::size_t index = 0; index < repeat_count; ++index) {
            updated.append(output_for_event(event, ends_with_bengali_consonant(updated)));
        }
        return append_output(updated);
    }

    layout::InputStatus backspace() override {
        if (composition_.empty()) {
            return layout::InputStatus::empty;
        }
        const auto graphemes = suborno::unicode::split_graphemes(composition_);
        if (graphemes.empty()) {
            composition_.clear();
            return layout::InputStatus::empty;
        }
        composition_.resize(composition_.size() - graphemes.back().size());
        return composition_.empty() ? layout::InputStatus::empty : layout::InputStatus::updated;
    }

    layout::CommitResult commit() override {
        if (composition_.empty()) {
            return {.text_utf8 = {}, .status = layout::InputStatus::empty};
        }
        std::string committed = std::move(composition_);
        composition_.clear();
        return {.text_utf8 = std::move(committed), .status = layout::InputStatus::updated};
    }

    layout::CommitResult commit_candidate(std::size_t) override {
        return {.text_utf8 = {}, .status = layout::InputStatus::candidate_out_of_range};
    }
    bool select_candidate(std::size_t) override {
        return false;
    }
    bool select_next_candidate() override {
        return false;
    }
    bool select_previous_candidate() noexcept override {
        return false;
    }
    std::size_t selected_candidate_index() const noexcept override {
        return 0;
    }
    void reset() noexcept override {
        composition_.clear();
    }
    layout::CompositionSnapshot snapshot() const override {
        layout::CompositionSnapshot result;
        result.raw_input_utf8 = composition_;
        result.preedit_utf8 = composition_;
        result.status =
            composition_.empty() ? layout::InputStatus::empty : layout::InputStatus::updated;
        return result;
    }

  private:
    layout::InputStatus append_output(const std::string& updated) {
        if (suborno::unicode::count_utf8_code_points(updated)
            > layout::InputSession::kMaxInputCodePoints) {
            return layout::InputStatus::input_limit;
        }
        composition_ = updated;
        return layout::InputStatus::updated;
    }

    std::string composition_;
};

}  // namespace

std::string ProbhatLayout::transliterate(const std::string& input) const {
    return transliterate_probhat(input);
}

std::vector<layout::Candidate> ProbhatLayout::generate_candidates(const std::string& input) const {
    const std::string output = transliterate_probhat(input);
    if (output.empty()) {
        return {};
    }
    return {{
        .text = output,
        .normalized_text = output,
        .stable_id = "layout:probhat:" + output,
        .source = layout::CandidateSource::layout,
        .score = 1.0,
        .rationale = "Probhat physical-key output",
    }};
}

std::unique_ptr<layout::InputSession> ProbhatLayout::create_session(
    dictionary::Dictionary*, layout::InputSessionOptions) const {
    return std::make_unique<ProbhatInputSession>();
}

}  // namespace suborno::layouts
