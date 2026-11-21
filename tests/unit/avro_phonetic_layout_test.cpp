#include "suborno/layouts/phonetic/avro_phonetic_layout.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

TEST(AvroPhoneticLayoutTest, ConvertsIndependentAndDependentVowels) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("a"), "আ");
    EXPECT_EQ(layout.transliterate("ka"), "কা");
    EXPECT_EQ(layout.transliterate("o"), "অ");
    EXPECT_EQ(layout.transliterate("ko"), "ক");
    EXPECT_EQ(layout.transliterate("O"), "ও");
    EXPECT_EQ(layout.transliterate("kO"), "কো");
    EXPECT_EQ(layout.transliterate("ee"), "ঈ");
    EXPECT_EQ(layout.transliterate("kee"), "কী");
    EXPECT_EQ(layout.transliterate("rri"), "ঋ");
    EXPECT_EQ(layout.transliterate("krri"), "কৃ");
    EXPECT_EQ(layout.transliterate("OI"), "ঐ");
    EXPECT_EQ(layout.transliterate("OU"), "ঔ");
}

TEST(AvroPhoneticLayoutTest, PreservesCaseSensitiveConsonantTokens) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("ch"), "চ");
    EXPECT_EQ(layout.transliterate("chh"), "ছ");
    EXPECT_EQ(layout.transliterate("t"), "ত");
    EXPECT_EQ(layout.transliterate("T"), "ট");
    EXPECT_EQ(layout.transliterate("th"), "থ");
    EXPECT_EQ(layout.transliterate("Th"), "ঠ");
    EXPECT_EQ(layout.transliterate("Ng"), "ঙ");
    EXPECT_EQ(layout.transliterate("NG"), "ঞ");
    EXPECT_EQ(layout.transliterate("ng"), "ং");
}

TEST(AvroPhoneticLayoutTest, HandlesAccentKeyOverrides) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("eki"), "একি");
    EXPECT_EQ(layout.transliterate(std::string("ek") + '\x60' + "i"), "একই");
    EXPECT_EQ(layout.transliterate(std::string("a") + '\x60'), "া");
    EXPECT_EQ(layout.transliterate(std::string("k") + '\x60' + "i"), "কই");
    EXPECT_EQ(layout.transliterate(std::string("k") + '\x60' + "ta"), "কতা");
}

TEST(AvroPhoneticLayoutTest, SupportsRephPhalaAndSpecialSigns) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("orrko"), "অর্ক");
    EXPECT_EQ(layout.transliterate("y"), "য়");
    EXPECT_EQ(layout.transliterate("by"), "\u09AC\u09CD\u09AF");
    EXPECT_EQ(layout.transliterate("kr"), "ক্র");
    EXPECT_EQ(layout.transliterate("km"), "ক্ম");
    EXPECT_EQ(layout.transliterate("rri"), "ঋ");
    EXPECT_EQ(layout.transliterate("ng"), "ং");
    EXPECT_EQ(layout.transliterate(":"), "ঃ");
    EXPECT_EQ(layout.transliterate("^"), "ঁ");
    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate("$"), "৳");
    EXPECT_EQ(layout.transliterate(std::string("t") + '\x60' + '\x60'), "ৎ");
}

TEST(AvroPhoneticLayoutTest, PreservesLiteralAndUnknownUnicodeText) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate(R"(\k)"), "k");
    EXPECT_EQ(layout.transliterate("hellO!"), "হেল্লো!");
    EXPECT_THROW(layout.transliterate("\xFF"), std::invalid_argument);
}
