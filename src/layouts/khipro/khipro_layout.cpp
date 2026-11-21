#include "suborno/layouts/khipro/khipro_layout.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "suborno/candidates/candidate_engine.hpp"
#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layouts/layout_input_session.hpp"
#include "suborno/unicode/bengali.hpp"

namespace suborno::layouts {
namespace {

enum class MappingKind {
    consonant,
    vowel,
    sign,
};

struct Mapping {
    std::string_view key;
    std::string_view independent;
    std::string_view dependent;
    std::string_view forced_dependent;
    MappingKind kind;
};

constexpr Mapping kMappings[] = {
    // Compositional consonants and their lowercase modifiers.
    {"tff", "ঠ", {}, {}, MappingKind::consonant},
    {"dff", "ঢ", {}, {}, MappingKind::consonant},
    {"nff", "ঞ", {}, {}, MappingKind::consonant},
    {"rff", "ঢ়", {}, {}, MappingKind::consonant},
    {"tf", "ট", {}, {}, MappingKind::consonant},
    {"df", "ড", {}, {}, MappingKind::consonant},
    {"nf", "ণ", {}, {}, MappingKind::consonant},
    {"sf", "ষ", {}, {}, MappingKind::consonant},
    {"rf", "ড়", {}, {}, MappingKind::consonant},
    {"kh", "খ", {}, {}, MappingKind::consonant},
    {"gh", "ঘ", {}, {}, MappingKind::consonant},
    {"ng", "ং", {}, {}, MappingKind::sign},
    {"ch", "ছ", {}, {}, MappingKind::consonant},
    {"jh", "ঝ", {}, {}, MappingKind::consonant},
    {"th", "থ", {}, {}, MappingKind::consonant},
    {"dh", "ধ", {}, {}, MappingKind::consonant},
    {"ph", "ফ", {}, {}, MappingKind::consonant},
    {"sh", "শ", {}, {}, MappingKind::consonant},
    {"k", "ক", {}, {}, MappingKind::consonant},
    {"g", "গ", {}, {}, MappingKind::consonant},
    {"c", "চ", {}, {}, MappingKind::consonant},
    {"j", "জ", {}, {}, MappingKind::consonant},
    {"t", "ত", {}, {}, MappingKind::consonant},
    {"d", "দ", {}, {}, MappingKind::consonant},
    {"n", "ন", {}, {}, MappingKind::consonant},
    {"p", "প", {}, {}, MappingKind::consonant},
    {"b", "ব", {}, {}, MappingKind::consonant},
    {"v", "ভ", {}, {}, MappingKind::consonant},
    {"m", "ম", {}, {}, MappingKind::consonant},
    {"z", "য", {}, {}, MappingKind::consonant},
    {"r", "র", {}, {}, MappingKind::consonant},
    {"l", "ল", {}, {}, MappingKind::consonant},
    {"s", "স", {}, {}, MappingKind::consonant},
    {"h", "হ", {}, {}, MappingKind::consonant},
    {"y", "য়", {}, {}, MappingKind::consonant},
    {"f", "ফ", {}, {}, MappingKind::consonant},

    // Vowels have independent and dependent forms. A trailing f forces the
    // two-part forms used by Khipro Classic (for example, amii / amiif).
    {"wa", "ওয়া", "োয়া", {}, MappingKind::vowel},
    {"ae", "অ্যা", "্যা", {}, MappingKind::vowel},
    {"wi", "ঐ", "ৈ", "োই", MappingKind::vowel},
    {"wu", "ঔ", "ৌ", "োউ", MappingKind::vowel},
    {"ii", "ঈ", "ী", "িই", MappingKind::vowel},
    {"uu", "ঊ", "ূ", "ুউ", MappingKind::vowel},
    {"q", "ঋ", "ৃ", {}, MappingKind::vowel},
    {"a", "আ", "া", {}, MappingKind::vowel},
    {"i", "ই", "ি", {}, MappingKind::vowel},
    {"u", "উ", "ু", {}, MappingKind::vowel},
    {"e", "এ", "ে", {}, MappingKind::vowel},
    {"w", "ও", "ো", {}, MappingKind::vowel},
    {"o", "অ", {}, {}, MappingKind::vowel},

    // Bengali signs.
    {"oo", "ঃ", {}, {}, MappingKind::sign},
    {"t/", "ৎ", {}, {}, MappingKind::sign},
    {"x", "ং", {}, {}, MappingKind::sign},
    {",,", "়", {}, {}, MappingKind::sign},
};

const Mapping* find_mapping(std::string_view input, std::size_t offset) noexcept {
    const Mapping* best = nullptr;
    for (const Mapping& mapping : kMappings) {
        if (mapping.key.size() > input.size() - offset) {
            continue;
        }
        if (input.compare(offset, mapping.key.size(), mapping.key) == 0
            && (best == nullptr || mapping.key.size() > best->key.size())) {
            best = &mapping;
        }
    }
    return best;
}

bool begins_with_consonant(std::string_view input, std::size_t offset) noexcept {
    const Mapping* mapping = find_mapping(input, offset);
    return mapping != nullptr && mapping->kind == MappingKind::consonant;
}

std::string transliterate_khipro(std::string_view input) {
    if (!suborno::unicode::is_valid_utf8(input)) {
        return {};
    }

    std::string output;
    output.reserve(input.size() * 3U);
    std::size_t offset = 0;
    std::size_t last_joiner_offset = std::string::npos;
    bool previous_was_consonant = false;
    bool previous_was_vowel = false;

    while (offset < input.size()) {
        if (input.compare(offset, 2, ";;") == 0) {
            output.push_back(';');
            offset += 2;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }

        if (input[offset] == ';') {
            ++offset;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }

        if (input[offset] == '/') {
            if (previous_was_vowel) {
                output.append("ঁ");
                previous_was_consonant = false;
                previous_was_vowel = false;
                last_joiner_offset = std::string::npos;
            } else if (previous_was_consonant && last_joiner_offset != std::string::npos) {
                output.erase(last_joiner_offset, std::string_view("্").size());
                last_joiner_offset = std::string::npos;
            } else {
                previous_was_consonant = false;
                previous_was_vowel = false;
                last_joiner_offset = std::string::npos;
            }
            ++offset;
            continue;
        }

        if (input.compare(offset, 3, "...") == 0) {
            output.append("…");
            offset += 3;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }
        if (input.compare(offset, 2, ".f") == 0) {
            output.append("॥");
            offset += 2;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }
        if (input.compare(offset, 2, "..") == 0) {
            output.push_back('.');
            offset += 2;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }
        if (input[offset] == '.') {
            output.append("।");
            ++offset;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }

        if (input.compare(offset, 2, "$$") == 0) {
            output.push_back('$');
            offset += 2;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }
        if (input.compare(offset, 2, "$f") == 0) {
            output.append("₹");
            offset += 2;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }
        if (input[offset] == '$') {
            output.append("৳");
            ++offset;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }
        if (input.compare(offset, 2, "+f") == 0) {
            output.append("×");
            offset += 2;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }
        if (input.compare(offset, 2, "-f") == 0) {
            output.append("÷");
            offset += 2;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }
        if (input.compare(offset, 2, "=f") == 0) {
            output.append("≠");
            offset += 2;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }

        const Mapping* mapping = find_mapping(input, offset);
        if (mapping == nullptr) {
            output.push_back(input[offset]);
            ++offset;
            previous_was_consonant = false;
            previous_was_vowel = false;
            last_joiner_offset = std::string::npos;
            continue;
        }

        const std::size_t next_offset = offset + mapping->key.size();
        if (mapping->kind == MappingKind::consonant) {
            if (previous_was_consonant) {
                last_joiner_offset = output.size();
                output.append("্");
            } else {
                last_joiner_offset = std::string::npos;
            }
            output.append(mapping->independent);
            previous_was_consonant = true;
            previous_was_vowel = false;
            offset = next_offset;
            continue;
        }

        if (mapping->kind == MappingKind::vowel) {
            const bool force_split = !mapping->forced_dependent.empty()
                                     && next_offset < input.size() && input[next_offset] == 'f';
            if (mapping->key == "o" && previous_was_consonant
                && begins_with_consonant(input, next_offset)) {
                previous_was_consonant = false;
                previous_was_vowel = false;
                last_joiner_offset = std::string::npos;
                offset = next_offset;
                continue;
            }

            if (previous_was_consonant) {
                if (force_split) {
                    output.append(mapping->forced_dependent);
                } else {
                    output.append(mapping->dependent);
                }
            } else {
                output.append(mapping->independent);
            }
            offset = next_offset + static_cast<std::size_t>(force_split);
            previous_was_consonant = false;
            previous_was_vowel = true;
            last_joiner_offset = std::string::npos;
            continue;
        }

        output.append(mapping->independent);
        offset = next_offset;
        previous_was_consonant = false;
        previous_was_vowel = false;
        last_joiner_offset = std::string::npos;
    }

    return suborno::unicode::normalize_nfc(output);
}

class KhiproInputSession final : public layout::InputSession {
  public:
    KhiproInputSession(const layout::InputLayout& input_layout,
                       dictionary::Dictionary* dictionary,
                       layout::InputSessionOptions options) {
        if (dictionary != nullptr) {
            state_ = std::make_unique<LayoutInputSession>(input_layout, *dictionary, options);
        } else {
            state_ = std::make_unique<LayoutInputSession>(input_layout, options);
        }
    }

    layout::InputStatus append_utf8(std::string_view input) override {
        return state_->append_utf8(input);
    }
    layout::InputStatus append_key(const layout::KeyEvent& event) override {
        return state_->append_key(event);
    }
    layout::InputStatus backspace() override {
        return state_->backspace();
    }
    layout::CommitResult commit() override {
        return state_->commit();
    }
    layout::CommitResult commit_candidate(std::size_t index) override {
        return state_->commit_candidate(index);
    }
    bool select_candidate(std::size_t index) override {
        return state_->select_candidate(index);
    }
    bool select_next_candidate() override {
        return state_->select_next_candidate();
    }
    bool select_previous_candidate() noexcept override {
        return state_->select_previous_candidate();
    }
    std::size_t selected_candidate_index() const noexcept override {
        return state_->selected_candidate_index();
    }
    void reset() noexcept override {
        state_->reset();
    }
    layout::CompositionSnapshot snapshot() const override {
        return state_->snapshot();
    }

  private:
    std::unique_ptr<LayoutInputSession> state_;
};

}  // namespace

std::string KhiproLayout::transliterate(const std::string& input) const {
    return transliterate_khipro(input);
}

std::vector<layout::Candidate> KhiproLayout::generate_candidates(const std::string& input) const {
    const std::string output = transliterate_khipro(input);
    if (output.empty()) {
        return {};
    }
    return {{
        .text = output,
        .normalized_text = output,
        .stable_id = "layout:khipro:" + output,
        .source = layout::CandidateSource::layout,
        .score = 1.0,
        .rationale = "Khipro Classic compositional output",
    }};
}

std::unique_ptr<layout::InputSession> KhiproLayout::create_session(
    dictionary::Dictionary* dictionary, layout::InputSessionOptions options) const {
    return std::make_unique<KhiproInputSession>(*this, dictionary, options);
}

}  // namespace suborno::layouts
