#include "suborno/unicode/bengali.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unicode/brkiter.h>
#include <unicode/locid.h>
#include <unicode/normalizer2.h>
#include <unicode/stringpiece.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>
#include <utility>

namespace suborno::unicode {
namespace {

icu::UnicodeString to_unicode_string(std::string_view text) {
    if (!is_valid_utf8(text)) {
        throw std::invalid_argument("Input is not valid UTF-8");
    }
    if (text.size() > static_cast<std::size_t>(INT32_MAX)) {
        throw std::length_error("Input exceeds ICU's maximum string length");
    }

    return icu::UnicodeString::fromUTF8(
        icu::StringPiece(text.data(), static_cast<int32_t>(text.size())));
}

std::string normalize(std::string_view text, bool compatibility) {
    if (text.empty()) {
        return {};
    }

    UErrorCode status = U_ZERO_ERROR;
    const icu::Normalizer2* normalizer = compatibility ? icu::Normalizer2::getNFKCInstance(status)
                                                       : icu::Normalizer2::getNFCInstance(status);
    if (U_FAILURE(status) != 0 || normalizer == nullptr) {
        throw std::runtime_error("Unable to initialize ICU normalizer");
    }

    const icu::UnicodeString source = to_unicode_string(text);
    icu::UnicodeString normalized;
    normalizer->normalize(source, normalized, status);
    if (U_FAILURE(status) != 0) {
        throw std::runtime_error("ICU normalization failed");
    }

    std::string converted;
    normalized.toUTF8String(converted);
    return converted;
}

}  // namespace

bool is_valid_utf8(std::string_view text) noexcept {
    if (text.size() > static_cast<std::size_t>(INT32_MAX)) {
        return false;
    }
    if (text.empty()) {
        return true;
    }

    UErrorCode status = U_ZERO_ERROR;
    int32_t utf16_length = 0;
    u_strFromUTF8(
        nullptr, 0, &utf16_length, text.data(), static_cast<int32_t>(text.size()), &status);
    return status == U_BUFFER_OVERFLOW_ERROR || U_SUCCESS(status) != 0;
}

std::size_t count_utf8_code_points(std::string_view text) {
    if (!is_valid_utf8(text)) {
        throw std::invalid_argument("Input is not valid UTF-8");
    }

    std::size_t count = 0;
    for (const unsigned char byte : text) {
        if (U8_IS_TRAIL(byte) == 0) {
            ++count;
        }
    }
    return count;
}

std::string normalize_nfc(std::string_view text) {
    return normalize(text, false);
}

std::string normalize_nfkc(const std::string& text) {
    return normalize(text, true);
}

std::vector<BengaliCharacterInfo> supported_bengali_inventory() {
    return {{"ঀ", "Bengali Anji", "Lo", "", "ঀ"},
            {"অ", "Bengali Letter A", "Lo", "a", "অ"},
            {"আ", "Bengali Letter AA", "Lo", "aa", "আ"},
            {"ই", "Bengali Letter I", "Lo", "i", "ই"},
            {"ঈ", "Bengali Letter II", "Lo", "ii", "ঈ"},
            {"উ", "Bengali Letter U", "Lo", "u", "উ"},
            {"ঊ", "Bengali Letter UU", "Lo", "uu", "ঊ"},
            {"ঋ", "Bengali Letter Vocalic R", "Lo", "ri", "ঋ"},
            {"ঌ", "Bengali Letter Vocalic L", "Lo", "", "ঌ"},
            {"এ", "Bengali Letter E", "Lo", "e", "এ"},
            {"ঐ", "Bengali Letter AI", "Lo", "oi", "ঐ"},
            {"ও", "Bengali Letter O", "Lo", "o", "ও"},
            {"ঔ", "Bengali Letter AU", "Lo", "ou", "ঔ"},
            {"ৠ", "Bengali Letter Vocalic RR", "Lo", "", "ৠ"},
            {"ৡ", "Bengali Letter Vocalic LL", "Lo", "", "ৡ"},
            {"ক", "Bengali Letter Ka", "Lo", "k", "ক"},
            {"খ", "Bengali Letter Kha", "Lo", "kh", "খ"},
            {"গ", "Bengali Letter Ga", "Lo", "g", "গ"},
            {"ঘ", "Bengali Letter Gha", "Lo", "gh", "ঘ"},
            {"চ", "Bengali Letter Ca", "Lo", "c", "চ"},
            {"ছ", "Bengali Letter Cha", "Lo", "ch", "ছ"},
            {"জ", "Bengali Letter Ja", "Lo", "j", "জ"},
            {"ঝ", "Bengali Letter Jha", "Lo", "jh", "ঝ"},
            {"ট", "Bengali Letter Tta", "Lo", "tt", "ট"},
            {"ঠ", "Bengali Letter Ttha", "Lo", "tth", "ঠ"},
            {"ড", "Bengali Letter Dda", "Lo", "dd", "ড"},
            {"ঢ", "Bengali Letter Ddha", "Lo", "ddh", "ঢ"},
            {"ণ", "Bengali Letter Nna", "Lo", "nn", "ণ"},
            {"ত", "Bengali Letter Ta", "Lo", "t", "ত"},
            {"থ", "Bengali Letter Tha", "Lo", "th", "থ"},
            {"দ", "Bengali Letter Da", "Lo", "d", "দ"},
            {"ধ", "Bengali Letter Dha", "Lo", "dh", "ধ"},
            {"ন", "Bengali Letter Na", "Lo", "n", "ন"},
            {"প", "Bengali Letter Pa", "Lo", "p", "প"},
            {"ফ", "Bengali Letter Pha", "Lo", "ph", "ফ"},
            {"ব", "Bengali Letter Ba", "Lo", "b", "ব"},
            {"ভ", "Bengali Letter Bha", "Lo", "bh", "ভ"},
            {"ম", "Bengali Letter Ma", "Lo", "m", "ম"},
            {"য", "Bengali Letter Ya", "Lo", "y", "য"},
            {"র", "Bengali Letter Ra", "Lo", "r", "র"},
            {"ল", "Bengali Letter La", "Lo", "l", "ল"},
            {"শ", "Bengali Letter Sha", "Lo", "sh", "শ"},
            {"ষ", "Bengali Letter Ssa", "Lo", "shh", "ষ"},
            {"স", "Bengali Letter Sa", "Lo", "s", "স"},
            {"হ", "Bengali Letter Ha", "Lo", "h", "হ"},
            {"ড়", "Bengali Letter RRA", "Lo", "R", "\u09A1\u09BC"},
            {"ঢ়", "Bengali Letter RHA", "Lo", "Rh", "\u09A2\u09BC"},
            {"য়", "Bengali Letter YYA", "Lo", "y", "\u09AF\u09BC"},
            {"ঙ", "Bengali Letter Nga", "Lo", "ng", "ঙ"},
            {"ঞ", "Bengali Letter Nya", "Lo", "ny", "ঞ"},

            {"ঁ", "Bengali Sign Candrabindu", "Mn", "^", "ঁ"},
            {"ং", "Bengali Sign Anusvara", "Mc", "ng", "ং"},
            {"়", "Bengali Sign Nukta", "Mn", "", "়"},
            {"ঽ", "Bengali Sign Avagraha", "Lo", "", "ঽ"},
            {"ঃ", "Bengali Sign Visarga", "Mc", "h", "ঃ"},
            {"ৎ", "Bengali Letter Khanda Ta", "Lo", "X", "ৎ"},
            {"্", "Bengali Sign Virama", "Mn", "", "্"},
            {"া", "Bengali Vowel Sign Aa", "Mc", "a", "া"},
            {"ি", "Bengali Vowel Sign I", "Mc", "i", "ি"},
            {"ী", "Bengali Vowel Sign II", "Mc", "ii", "ী"},
            {"ু", "Bengali Vowel Sign U", "Mn", "u", "ু"},
            {"ূ", "Bengali Vowel Sign UU", "Mn", "uu", "ূ"},
            {"ৃ", "Bengali Vowel Sign Vocalic R", "Mn", "ri", "ৃ"},
            {"ৄ", "Bengali Vowel Sign Vocalic RR", "Mn", "", "ৄ"},
            {"ে", "Bengali Vowel Sign E", "Mc", "e", "ে"},
            {"ৈ", "Bengali Vowel Sign AI", "Mc", "oi", "ৈ"},
            {"ো", "Bengali Vowel Sign O", "Mc", "o", "ো"},
            {"ৌ", "Bengali Vowel Sign AU", "Mc", "ou", "ৌ"},
            {"ৗ", "Bengali AU Length Mark", "Mc", "", "ৗ"},
            {"ৢ", "Bengali Vowel Sign Vocalic L", "Mn", "", "ৢ"},
            {"ৣ", "Bengali Vowel Sign Vocalic LL", "Mn", "", "ৣ"},
            {"৲", "Bengali Rupee Mark", "Sc", "", "৲"},
            {"৳", "Bengali Rupee Sign", "Sc", "rs", "৳"},
            {"৴", "Bengali Currency Numerator One", "No", "", "৴"},
            {"৵", "Bengali Currency Numerator Two", "No", "", "৵"},
            {"৶", "Bengali Currency Numerator Three", "No", "", "৶"},
            {"৷", "Bengali Currency Numerator Four", "No", "", "৷"},
            {"৸", "Bengali Currency Numerator One Less Than The Denominator", "No", "", "৸"},
            {"৹", "Bengali Currency Denominator Sixteen", "No", "", "৹"},
            {"৺", "Bengali Isshar", "So", "", "৺"},
            {"৻", "Bengali Ganda Mark", "Sc", "", "৻"},
            {"ৼ", "Bengali Letter Vedic Anusvara", "Lo", "", "ৼ"},
            {"৽", "Bengali Abbreviation Sign", "Po", "", "৽"},
            {"৾", "Bengali Sandhi Mark", "Mn", "", "৾"},
            {"০", "Bengali Digit Zero", "Nd", "0", "০"},
            {"১", "Bengali Digit One", "Nd", "1", "১"},
            {"২", "Bengali Digit Two", "Nd", "2", "২"},
            {"৩", "Bengali Digit Three", "Nd", "3", "৩"},
            {"৪", "Bengali Digit Four", "Nd", "4", "৪"},
            {"৫", "Bengali Digit Five", "Nd", "5", "৫"},
            {"৬", "Bengali Digit Six", "Nd", "6", "৬"},
            {"৭", "Bengali Digit Seven", "Nd", "7", "৭"},
            {"৮", "Bengali Digit Eight", "Nd", "8", "৮"},
            {"৯", "Bengali Digit Nine", "Nd", "9", "৯"}};
}

bool is_bengali_codepoint(unsigned int code_point) {
    return (code_point >= 0x0980U && code_point <= 0x0983U)
           || (code_point >= 0x0985U && code_point <= 0x098CU)
           || (code_point >= 0x098FU && code_point <= 0x0990U)
           || (code_point >= 0x0993U && code_point <= 0x09A8U)
           || (code_point >= 0x09AAU && code_point <= 0x09B0U) || code_point == 0x09B2U
           || (code_point >= 0x09B6U && code_point <= 0x09B9U)
           || (code_point >= 0x09BCU && code_point <= 0x09BDU)
           || (code_point >= 0x09BEU && code_point <= 0x09C4U)
           || (code_point >= 0x09C7U && code_point <= 0x09C8U)
           || (code_point >= 0x09CBU && code_point <= 0x09CEU) || code_point == 0x09D7U
           || (code_point >= 0x09DCU && code_point <= 0x09DDU)
           || (code_point >= 0x09DFU && code_point <= 0x09E3U)
           || (code_point >= 0x09E6U && code_point <= 0x09EFU)
           || (code_point >= 0x09F2U && code_point <= 0x09FEU);
}

std::vector<std::string> split_graphemes(const std::string& text) {
    std::vector<std::string> result;
    if (text.empty()) {
        return result;
    }

    const icu::UnicodeString source = to_unicode_string(text);
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::BreakIterator> iterator(
        icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(), status));
    if (U_FAILURE(status) != 0 || iterator == nullptr) {
        throw std::runtime_error("Unable to initialize ICU grapheme iterator");
    }

    iterator->setText(source);
    int32_t start = iterator->first();
    for (int32_t end = iterator->next(); end != icu::BreakIterator::DONE;
         start = end, end = iterator->next()) {
        std::string grapheme;
        source.tempSubStringBetween(start, end).toUTF8String(grapheme);
        result.push_back(std::move(grapheme));
    }
    return result;
}

}  // namespace suborno::unicode
