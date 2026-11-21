#include "suborno/layouts/phonetic/avro_phonetic_layout.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string_view>
#include <utility>
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
    EXPECT_EQ(layout.transliterate("samia"), "সামিয়া");
    EXPECT_EQ(layout.transliterate("oi"), "অই");
    EXPECT_EQ(layout.transliterate("OI"), "ঐ");
    EXPECT_EQ(layout.transliterate("OU"), "ঔ");
}

TEST(AvroPhoneticLayoutTest, PreservesCaseSensitiveConsonantTokens) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("c"), "চ");
    EXPECT_EQ(layout.transliterate("ch"), "ছ");
    EXPECT_EQ(layout.transliterate("chh"), "ছ");
    EXPECT_EQ(layout.transliterate("j"), "জ");
    EXPECT_EQ(layout.transliterate("J"), "জ");
    EXPECT_EQ(layout.transliterate("y"), "ইয়");
    EXPECT_EQ(layout.transliterate("yahoo"), "ইয়াহু");
    EXPECT_EQ(layout.transliterate("Y"), "য়");
    EXPECT_EQ(layout.transliterate("Z"), "য");
    EXPECT_EQ(layout.transliterate("kY"), "কয়");
    EXPECT_EQ(layout.transliterate("kZ"), "ক্য");
    EXPECT_EQ(layout.transliterate("t"), "ত");
    EXPECT_EQ(layout.transliterate("T"), "ট");
    EXPECT_EQ(layout.transliterate("th"), "থ");
    EXPECT_EQ(layout.transliterate("Th"), "ঠ");
    EXPECT_EQ(layout.transliterate("Ng"), "ঙ");
    EXPECT_EQ(layout.transliterate("NG"), "ঞ");
    EXPECT_EQ(layout.transliterate("ng"), "ং");
}

TEST(AvroPhoneticLayoutTest, ComposesYaPhalaWithoutJoiningTheFollowingConsonant) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("by"), "ব্য");
    EXPECT_EQ(layout.transliterate("bybohar"), "ব্যবহার");
}

TEST(AvroPhoneticLayoutTest, HandlesAccentKeyOverrides) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("eki"), "একি");
    EXPECT_EQ(layout.transliterate(std::string("ek") + '\x60' + "i"), "একই");
    EXPECT_EQ(layout.transliterate(std::string("a") + '\x60'), "া");
    EXPECT_EQ(layout.transliterate(std::string("k") + '\x60' + "i"), "কই");
    EXPECT_EQ(layout.transliterate(std::string("k") + '\x60' + "ta"), "কতা");
    EXPECT_EQ(layout.transliterate(std::string(".") + '\x60'), ".");
    EXPECT_EQ(layout.transliterate(std::string(":") + '\x60'), ":");
}

TEST(AvroPhoneticLayoutTest, SupportsRephPhalaAndSpecialSigns) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("orrko"), "অর্ক");
    EXPECT_EQ(layout.transliterate("y"), "য়");
    EXPECT_EQ(layout.transliterate("by"), "\u09AC\u09CD\u09AF");
    EXPECT_EQ(layout.transliterate("kr"), "ক্র");
    EXPECT_EQ(layout.transliterate("km"), "কম");
    EXPECT_EQ(layout.transliterate("rri"), "ঋ");
    EXPECT_EQ(layout.transliterate("ng"), "ং");
    EXPECT_EQ(layout.transliterate(":"), "ঃ");
    EXPECT_EQ(layout.transliterate("^"), "ঁ");
    EXPECT_EQ(layout.transliterate("."), "।");
    EXPECT_EQ(layout.transliterate("$"), "৳");
    EXPECT_EQ(layout.transliterate(std::string("t") + '\x60' + '\x60'), "ৎ");
}

TEST(AvroPhoneticLayoutTest, MatchesTheDocumentedConjunctCorpus) {
    const suborno::layouts::AvroPhoneticLayout layout;
    constexpr std::pair<std::string_view, std::string_view> vectors[]{
        {"kk", "ক্ক"},
        {"kT", "ক্ট"},
        {"kt", "ক্ত"},
        {"ktr", "ক্ত্র"},
        {"kw", "ক্ব"},
        {"ky", "ক্য"},
        {"kZ", "ক্য"},
        {"kr", "ক্র"},
        {"kl", "ক্ল"},
        {"kkh", "ক্ষ"},
        {"kx", "ক্ষ"},
        {"kkhw", "ক্ষ্ব"},
        {"kxw", "ক্ষ্ব"},
        {"kkhN", "ক্ষ্ণ"},
        {"kxN", "ক্ষ্ণ"},
        {"kkhm", "ক্ষ্ম"},
        {"kxm", "ক্ষ্ম"},
        {"kkhy", "ক্ষ্য"},
        {"kxy", "ক্ষ্য"},
        {"kkhZ", "ক্ষ্য"},
        {"kxZ", "ক্ষ্য"},
        {"ks", "ক্স"},
        {"khy", "খ্য"},
        {"khZ", "খ্য"},
        {"khr", "খ্র"},
        {"gN", "গ্ণ"},
        {"gdh", "গ্ধ"},
        {"gn", "গ্ন"},
        {"gny", "গ্ন্য"},
        {"gnZ", "গ্ন্য"},
        {"gw", "গ্ব"},
        {"gm", "গ্ম"},
        {"gy", "গ্য"},
        {"gZ", "গ্য"},
        {"gr", "গ্র"},
        {"gl", "গ্ল"},
        {"ghn", "ঘ্ন"},
        {"ghy", "ঘ্য"},
        {"ghZ", "ঘ্য"},
        {"ghr", "ঘ্র"},
        {"nk", "ঙ্ক"},
        {"Ngk", "ঙ্ক"},
        {"nky", "ঙ্ক্য"},
        {"Ngky", "ঙ্ক্য"},
        {"nkZ", "ঙ্ক্য"},
        {"NgkZ", "ঙ্ক্য"},
        {"Ngkkh", "ঙ্ক্ষ"},
        {"Ngkx", "ঙ্ক্ষ"},
        {"Ngkh", "ঙ্খ"},
        {"Ngg", "ঙ্গ"},
        {"Nggy", "ঙ্গ্য"},
        {"NggZ", "ঙ্গ্য"},
        {"Nggh", "ঙ্ঘ"},
        {"Ngghy", "ঙ্ঘ্য"},
        {"NgghZ", "ঙ্ঘ্য"},
        {"Ngghr", "ঙ্ঘ্র"},
        {"Ngm", "ঙ্ম"},
        {"cc", "চ্চ"},
        {"cch", "চ্ছ"},
        {"cchw", "চ্ছ্ব"},
        {"cchr", "চ্ছ্র"},
        {"cNG", "চ্ঞ"},
        {"cy", "চ্য"},
        {"cZ", "চ্য"},
        {"jj", "জ্জ"},
        {"jjw", "জ্জ্ব"},
        {"jjh", "জ্ঝ"},
        {"gg", "জ্ঞ"},
        {"jNG", "জ্ঞ"},
        {"jw", "জ্ব"},
        {"jy", "জ্য"},
        {"jZ", "জ্য"},
        {"jr", "জ্র"},
        {"nc", "ঞ্চ"},
        {"NGc", "ঞ্চ"},
        {"nch", "ঞ্ছ"},
        {"NGch", "ঞ্ছ"},
        {"nj", "ঞ্জ"},
        {"NGj", "ঞ্জ"},
        {"njh", "ঞ্ঝ"},
        {"NGjh", "ঞ্ঝ"},
        {"TT", "ট্ট"},
        {"Tw", "ট্ব"},
        {"Tm", "ট্ম"},
        {"Ty", "ট্য"},
        {"TZ", "ট্য"},
        {"Tr", "ট্র"},
        {"DD", "ড্ড"},
        {"Dy", "ড্য"},
        {"DZ", "ড্য"},
        {"Dr", "ড্র"},
        {"Dhy", "ঢ্য"},
        {"DhZ", "ঢ্য"},
        {"Dhr", "ঢ্র"},
        {"NT", "ণ্ট"},
        {"NTh", "ণ্ঠ"},
        {"ND", "ণ্ড"},
        {"NDy", "ণ্ড্য"},
        {"NDZ", "ণ্ড্য"},
        {"NDr", "ণ্ড্র"},
        {"NDh", "ণ্ঢ"},
        {"NN", "ণ্ণ"},
        {"Nw", "ণ্ব"},
        {"Nm", "ণ্ম"},
        {"Ny", "ণ্য"},
        {"NZ", "ণ্য"},
        {"tt", "ত্ত"},
        {"ttw", "ত্ত্ব"},
        {"tth", "ত্থ"},
        {"tn", "ত্ন"},
        {"tw", "ত্ব"},
        {"tm", "ত্ম"},
        {"tmy", "ত্ম্য"},
        {"tmZ", "ত্ম্য"},
        {"ty", "ত্য"},
        {"tZ", "ত্য"},
        {"tr", "ত্র"},
        {"thw", "থ্ব"},
        {"thy", "থ্য"},
        {"thZ", "থ্য"},
        {"thr", "থ্র"},
        {"dg", "দ্গ"},
        {"dgh", "দ্ঘ"},
        {"dd", "দ্দ"},
        {"ddw", "দ্দ্ব"},
        {"ddh", "দ্ধ"},
        {"dw", "দ্ব"},
        {"dv", "দ্ভ"},
        {"dbh", "দ্ভ"},
        {"dm", "দ্ম"},
        {"dy", "দ্য"},
        {"dZ", "দ্য"},
        {"dr", "দ্র"},
        {"dhn", "ধ্ন"},
        {"dhw", "ধ্ব"},
        {"dhm", "ধ্ম"},
        {"dhy", "ধ্য"},
        {"dhZ", "ধ্য"},
        {"dhr", "ধ্র"},
        {"nT", "ন্ট"},
        {"nTh", "ন্ঠ"},
        {"nD", "ন্ড"},
        {"nt", "ন্ত"},
        {"ntw", "ন্ত্ব"},
        {"nty", "ন্ত্য"},
        {"ntZ", "ন্ত্য"},
        {"ntr", "ন্ত্র"},
        {"nth", "ন্থ"},
        {"nd", "ন্দ"},
        {"ndy", "ন্দ্য"},
        {"ndZ", "ন্দ্য"},
        {"ndw", "ন্দ্ব"},
        {"ndr", "ন্দ্র"},
        {"ndh", "ন্ধ"},
        {"ndhy", "ন্ধ্য"},
        {"ndhZ", "ন্ধ্য"},
        {"ndhr", "ন্ধ্র"},
        {"nn", "ন্ন"},
        {"nw", "ন্ব"},
        {"nm", "ন্ম"},
        {"ny", "ন্য"},
        {"nZ", "ন্য"},
        {"ns", "ন্স"},
        {"pT", "প্ট"},
        {"pt", "প্ত"},
        {"pn", "প্ন"},
        {"pp", "প্প"},
        {"py", "প্য"},
        {"pZ", "প্য"},
        {"pr", "প্র"},
        {"pl", "প্ল"},
        {"ps", "প্স"},
        {"fr", "ফ্র"},
        {"phr", "ফ্র"},
        {"fl", "ফ্ল"},
        {"phl", "ফ্ল"},
        {"bj", "ব্জ"},
        {"bd", "ব্দ"},
        {"bdh", "ব্ধ"},
        {"bb", "ব্ব"},
        {"by", "ব্য"},
        {"bZ", "ব্য"},
        {"br", "ব্র"},
        {"bl", "ব্ল"},
        {"vy", "ভ্য"},
        {"vZ", "ভ্য"},
        {"bhy", "ভ্য"},
        {"bhZ", "ভ্য"},
        {"vr", "ভ্র"},
        {"bhr", "ভ্র"},
        {"vl", "ভ্ল"},
        {"bhl", "ভ্ল"},
        {"mth", "ম্থ"},
        {"mn", "ম্ন"},
        {"mp", "ম্প"},
        {"mpr", "ম্প্র"},
        {"mf", "ম্ফ"},
        {"mph", "ম্ফ"},
        {"mb", "ম্ব"},
        {"mw", "ম্ব"},
        {"mv", "ম্ভ"},
        {"mbh", "ম্ভ"},
        {"mvr", "ম্ভ্র"},
        {"mbhr", "ম্ভ্র"},
        {"mm", "ম্ম"},
        {"my", "ম্য"},
        {"mZ", "ম্য"},
        {"mr", "ম্র"},
        {"ml", "ম্ল"},
        {"zy", "য্য"},
        {"zZ", "য্য"},
        {"rrk", "র্ক"},
        {"rrkh", "র্খ"},
        {"rrg", "র্গ"},
        {"rrky", "র্ক্য"},
        {"rrkZ", "র্ক্য"},
        {"rrkhy", "র্খ্য"},
        {"rrkhZ", "র্খ্য"},
        {"lk", "ল্ক"},
        {"lg", "ল্গ"},
        {"lT", "ল্ট"},
        {"lD", "ল্ড"},
        {"ldh", "ল্ধ"},
        {"lp", "ল্প"},
        {"lb", "ল্ব"},
        {"lw", "ল্ব"},
        {"lv", "ল্ভ"},
        {"lbh", "ল্ভ"},
        {"lm", "ল্ম"},
        {"ly", "ল্য"},
        {"lZ", "ল্য"},
        {"ll", "ল্ল"},
        {"shc", "শ্চ"},
        {"Sc", "শ্চ"},
        {"shch", "শ্ছ"},
        {"Sch", "শ্ছ"},
        {"sht", "শ্ত"},
        {"St", "শ্ত"},
        {"shn", "শ্ন"},
        {"Sn", "শ্ন"},
        {"shw", "শ্ব"},
        {"Sw", "শ্ব"},
        {"shm", "শ্ম"},
        {"Sm", "শ্ম"},
        {"shy", "শ্য"},
        {"shZ", "শ্য"},
        {"Sy", "শ্য"},
        {"SZ", "শ্য"},
        {"shr", "শ্র"},
        {"Sr", "শ্র"},
        {"shl", "শ্ল"},
        {"Sl", "শ্ল"},
        {"Shk", "ষ্ক"},
        {"Shkr", "ষ্ক্র"},
        {"ShT", "ষ্ট"},
        {"ShTy", "ষ্ট্য"},
        {"ShTZ", "ষ্ট্য"},
        {"ShTr", "ষ্ট্র"},
        {"ShTh", "ষ্ঠ"},
        {"ShThy", "ষ্ঠ্য"},
        {"ShThZ", "ষ্ঠ্য"},
        {"ShN", "ষ্ণ"},
        {"Shp", "ষ্প"},
        {"Shpr", "ষ্প্র"},
        {"Shph", "ষ্ফ"},
        {"Shf", "ষ্ফ"},
        {"Shw", "ষ্ব"},
        {"Shm", "ষ্ম"},
        {"sk", "স্ক"},
        {"skr", "স্ক্র"},
        {"skl", "স্ক্ল"},
        {"skh", "স্খ"},
        {"sT", "স্ট"},
        {"sTr", "স্ট্র"},
        {"st", "স্ত"},
        {"stw", "স্ত্ব"},
        {"sty", "স্ত্য"},
        {"stZ", "স্ত্য"},
        {"sth", "স্থ"},
        {"sthy", "স্থ্য"},
        {"sthZ", "স্থ্য"},
        {"sn", "স্ন"},
        {"sp", "স্প"},
        {"sf", "স্ফ"},
        {"sph", "স্ফ"},
        {"sw", "স্ব"},
        {"sm", "স্ম"},
        {"sy", "স্য"},
        {"sZ", "স্য"},
        {"sr", "স্র"},
        {"sl", "স্ল"},
        {"hN", "হ্ণ"},
        {"hn", "হ্ন"},
        {"hw", "হ্ব"},
        {"hm", "হ্ম"},
        {"hy", "হ্য"},
        {"hZ", "হ্য"},
        {"hr", "হ্র"},
        {"hl", "হ্ল"},
    };

    for (const auto& [input, expected] : vectors) {
        EXPECT_EQ(layout.transliterate(std::string(input)), expected) << input;
    }
}

TEST(AvroPhoneticLayoutTest, MatchesDocumentedConsonantSeparationExamples) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("okkhor"), "অক্ষর");
    EXPECT_EQ(layout.transliterate("onto"), "অন্ত");
    EXPECT_EQ(layout.transliterate("biggo"), "বিজ্ঞ");
    EXPECT_EQ(layout.transliterate("lokkhmI"), "লক্ষ্মী");
    EXPECT_EQ(layout.transliterate("brahmoN"), "ব্রাহ্মণ");
    EXPECT_EQ(layout.transliterate("korbo"), "করব");
    EXPECT_EQ(layout.transliterate("ekTa"), "এক্টা");
    EXPECT_EQ(layout.transliterate("ek" + std::string(1, '\x60') + "Ta"), "একটা");
    EXPECT_EQ(layout.transliterate("amra"), "আম্রা");
    EXPECT_EQ(layout.transliterate("am" + std::string(1, '\x60') + "ra"), "আমরা");
    EXPECT_EQ(layout.transliterate("aglanO"), "আগ্লানো");
    EXPECT_EQ(layout.transliterate("ag" + std::string(1, '\x60') + "lanO"), "আগলানো");
}

TEST(AvroPhoneticLayoutTest, PreservesLiteralAndUnknownUnicodeText) {
    const suborno::layouts::AvroPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate(R"(\k)"), "k");
    EXPECT_EQ(layout.transliterate("hellO!"), "হেল্লো!");
    EXPECT_THROW(layout.transliterate("\xFF"), std::invalid_argument);
}
