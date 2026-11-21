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
    std::uint32_t scan_code;
    std::string_view unshifted;
    std::string_view shifted;
    bool caps_lock_toggles_shift;
};

// Base, Shift, and CapsLock layers follow the MIT-licensed Probhat KLC table.
constexpr ProbhatKeyMapping kProbhatMappings[] = {
    {0x02, "১", "!", false},
    {0x03, "২", "@", false},
    {0x04, "৩", "#", false},
    {0x05, "৪", "৳", false},
    {0x06, "৫", "%", false},
    {0x07, "৬", "^", false},
    {0x08, "৭", "ঞ", false},
    {0x09, "৮", "ৎ", false},
    {0x0A, "৯", "(", false},
    {0x0B, "০", ")", false},
    {0x0C, "-", "_", false},
    {0x0D, "=", "+", false},
    {0x10, "দ", "ধ", true},
    {0x11, "ূ", "ঊ", true},
    {0x12, "ী", "ঈ", true},
    {0x13, "র", "ড়", true},
    {0x14, "ট", "ঠ", true},
    {0x15, "এ", "ঐ", true},
    {0x16, "ু", "উ", true},
    {0x17, "ি", "ই", true},
    {0x18, "ও", "ঔ", true},
    {0x19, "প", "ফ", true},
    {0x1A, "ে", "ৈ", false},
    {0x1B, "ো", "ৌ", false},
    {0x1E, "া", "অ", true},
    {0x1F, "স", "ষ", true},
    {0x20, "ড", "ঢ", true},
    {0x21, "ত", "থ", true},
    {0x22, "গ", "ঘ", true},
    {0x23, "হ", "ঃ", true},
    {0x24, "জ", "ঝ", true},
    {0x25, "ক", "খ", true},
    {0x26, "ল", "ং", true},
    {0x27, ";", ":", false},
    {0x28, "'", "\"", false},
    {0x29, "\x60", "~", false},
    {0x2B, "\\", "|", false},
    {0x2C, "য়", "য", true},
    {0x2D, "শ", "ঢ়", true},
    {0x2E, "চ", "ছ", true},
    {0x2F, "আ", "ঋ", true},
    {0x30, "ব", "ভ", true},
    {0x31, "ন", "ণ", true},
    {0x32, "ম", "ঙ", true},
    {0x33, ",", "ৃ", true},
    {0x34, "।", "ঁ", false},
    {0x35, "্", "?", false},
    {0x39, " ", " ", false},
    {0x53, ".", ".", false},
    {0x56, "\\", "|", false},
};

const ProbhatKeyMapping* find_mapping(std::uint32_t scan_code) noexcept {
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

    struct PunctuationKey {
        char unshifted;
        char shifted;
        std::uint8_t scan_code;
    };
    constexpr PunctuationKey kPunctuation[]{
        {'!', '1', 0x02}, {'@', '2', 0x03}, {'#', '3', 0x04}, {'$', '4', 0x05},
        {'%', '5', 0x06}, {'^', '6', 0x07}, {'&', '7', 0x08}, {'*', '8', 0x09},
        {'(', '9', 0x0A}, {')', '0', 0x0B}, {'-', '_', 0x0C}, {'=', '+', 0x0D},
        {'[', '{', 0x1A}, {']', '}', 0x1B}, {';', ':', 0x27}, {'\'', '"', 0x28},
        {'\x60', '~', 0x29}, {'\\', '|', 0x2B}, {',', '<', 0x33}, {'.', '>', 0x34},
        {'/', '?', 0x35}, {' ', ' ', 0x39},
    };
    for (const PunctuationKey& key : kPunctuation) {
        if (value == key.unshifted || value == key.shifted) {
            event.scan_code = key.scan_code;
            event.modifiers.shift = value == key.shifted;
            return true;
        }
    }
    return false;
}

std::string output_for_event(const layout::KeyEvent& event) {
    if (!event.key_down || event.modifiers.control || event.modifiers.alt
        || event.modifiers.windows) {
        return {};
    }

    const ProbhatKeyMapping* mapping = find_mapping(event.scan_code);
    if (mapping == nullptr) {
        return {};
    }

    const bool shift_active =
        event.modifiers.shift
        != (mapping->caps_lock_toggles_shift && event.modifiers.caps_lock);
    return std::string(shift_active ? mapping->shifted : mapping->unshifted);
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
            output.append(output_for_event(event));
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
                updated.append(output_for_event(event));
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
        const std::string output = output_for_event(event);
        if (output.empty()) {
            return layout::InputStatus::empty;
        }
        const std::size_t repeat_count = event.repeat_count == 0 ? 1 : event.repeat_count;
        if (repeat_count > layout::InputSession::kMaxInputCodePoints) {
            return layout::InputStatus::input_limit;
        }

        std::string updated = composition_;
        for (std::size_t index = 0; index < repeat_count; ++index) {
            updated.append(output);
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
