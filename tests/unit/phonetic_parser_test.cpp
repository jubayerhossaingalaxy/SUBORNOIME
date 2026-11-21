#include "layouts/phonetic/phonetic_parser.hpp"

#include <gtest/gtest.h>

#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

TEST(PhoneticParserTest, AppliesIndependentAndDependentVowelStateTransitions) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("a"), "আ");
    EXPECT_EQ(layout.transliterate("ka"), "কা");
    EXPECT_EQ(layout.transliterate("ô"), "অ");
    EXPECT_EQ(layout.transliterate("kô"), "ক");
    EXPECT_EQ(layout.transliterate("kala"), "কালা");
    EXPECT_EQ(layout.transliterate("kôla"), "কলা");
}

TEST(PhoneticParserTest, PreservesCaseSensitiveRetroflexMappings) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("t"), "ত");
    EXPECT_EQ(layout.transliterate("T"), "ট");
    EXPECT_EQ(layout.transliterate("th"), "থ");
    EXPECT_EQ(layout.transliterate("Th"), "ঠ");
    EXPECT_EQ(layout.transliterate("d"), "দ");
    EXPECT_EQ(layout.transliterate("D"), "ড");
    EXPECT_EQ(layout.transliterate("dh"), "ধ");
    EXPECT_EQ(layout.transliterate("Dh"), "ঢ");
    EXPECT_EQ(layout.transliterate("n"), "ন");
    EXPECT_EQ(layout.transliterate("N"), "ণ");
}

TEST(PhoneticParserTest, AppliesNasalLookaheadBeforeOrdinaryTokenConsumption) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("ng"), "ং");
    EXPECT_EQ(layout.transliterate("nga"), "ঙা");
    EXPECT_EQ(layout.transliterate("ngi"), "ঙি");
    EXPECT_EQ(layout.transliterate("ngk"), "ংক");
    EXPECT_EQ(layout.transliterate("nkh"), "ংখ");
    EXPECT_EQ(layout.transliterate("nk"), "ংক");
    EXPECT_EQ(layout.transliterate("ngg"), "ংগ");
    EXPECT_EQ(layout.transliterate("ngh"), "ংহ");
    EXPECT_EQ(layout.transliterate("ngkh"), "ংখ");
    EXPECT_EQ(layout.transliterate("ngk"), "ংক");
    EXPECT_EQ(layout.transliterate("nk"), "ংক");
    EXPECT_EQ(layout.transliterate("nkh"), "ংখ");
    EXPECT_EQ(layout.transliterate("ng"), "ং");
    EXPECT_EQ(layout.transliterate("sonkha"), "সোংখা");
    EXPECT_EQ(layout.transliterate("ngka"), "ংকা");
    EXPECT_EQ(layout.transliterate("ngga"), "ংগা");
    EXPECT_EQ(layout.transliterate("nggha"), "ংঘা");
}

TEST(PhoneticParserTest, NasalAlternatesAreNotGeneratedForNgkOrNk) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.generate_candidates("ngk").size(), 1U);
    EXPECT_EQ(layout.generate_candidates("ngk").front().text, "ংক");
    EXPECT_EQ(layout.generate_candidates("nk").size(), 1U);
    EXPECT_EQ(layout.generate_candidates("nk").front().text, "ংক");
}

TEST(PhoneticParserTest, TreatsMAndHAsSignsWithoutAddingVirama) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("M"), "ং");
    EXPECT_EQ(layout.transliterate("H"), "ঃ");
    EXPECT_EQ(layout.transliterate("kM"), "কং");
    EXPECT_EQ(layout.transliterate("kH"), "কঃ");
    EXPECT_EQ(layout.transliterate("kaM"), "কাং");
    EXPECT_EQ(layout.transliterate("kaH"), "কাঃ");
}

TEST(PhoneticParserTest, AppliesExplicitControlTransitionsAndModifiers) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate(":danda:"), "\u0964");
    EXPECT_EQ(layout.transliterate(":ddanda:"), "\u0965");
    EXPECT_EQ(layout.transliterate("k:halant:"), "\u0995\u09CD");
    EXPECT_EQ(layout.transliterate("k:halant:t"), "\u0995\u09CD\u09A4");
    EXPECT_EQ(layout.transliterate("k:halant:a"), "\u0995\u09CD\u0986");
    EXPECT_EQ(layout.transliterate("k:halant-zwnj:"), "\u0995\u09CD\u200C");
    EXPECT_EQ(layout.transliterate("k:halant-zwj:"), "\u0995\u09CD\u200D");
    EXPECT_EQ(layout.transliterate("k:halant::zwnj:"), "\u0995\u09CD\u200C");
    EXPECT_EQ(layout.transliterate("k:halant::zwj:"), "\u0995\u09CD\u200D");
    EXPECT_EQ(layout.transliterate("k:halant-zwnj:t"), "\u0995\u09CD\u200Cত");
    EXPECT_EQ(layout.transliterate("k:boundary:a"), "কআ");
    EXPECT_EQ(layout.transliterate(":zwnj:"), ":zwnj:");
    EXPECT_EQ(layout.transliterate(":zwj:"), ":zwj:");
    EXPECT_EQ(layout.transliterate(":halant:"), ":halant:");
    EXPECT_EQ(layout.transliterate(":nukta:"), ":nukta:");
    EXPECT_EQ(layout.transliterate("D:nukta:"), "\u09A1\u09BC");
    EXPECT_EQ(layout.transliterate("D:nukta::nukta:"), "\u09A1\u09BC:nukta:");
    EXPECT_EQ(layout.transliterate("k:halant:M"), "\u0995\u09CDM");
}

TEST(PhoneticParserTest, ExercisesValidAndInvalidControlContexts) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("k:boundary:t"), "কত");
    EXPECT_EQ(layout.transliterate("a:boundary:i"), "আই");

    EXPECT_EQ(layout.transliterate("k:danda:t"), "ক\u0964ত");
    EXPECT_EQ(layout.transliterate("ka:ddanda:k"), "কা\u0965ক");

    EXPECT_EQ(layout.transliterate("k:halant:"), "ক\u09CD");
    EXPECT_EQ(layout.transliterate("k:halant:t"), "ক\u09CDত");
    EXPECT_EQ(layout.transliterate("k:halant:a"), "ক\u09CDআ");
    EXPECT_EQ(layout.transliterate(":halant:"), ":halant:");
    EXPECT_EQ(layout.transliterate("a:halant:"), "আ:halant:");

    EXPECT_EQ(layout.transliterate("k:halant-zwnj:t"), "ক\u09CD\u200Cত");
    EXPECT_EQ(layout.transliterate("k:halant-zwj:t"), "ক\u09CD\u200Dত");
    EXPECT_EQ(layout.transliterate(":halant-zwnj:"), ":halant-zwnj:");
    EXPECT_EQ(layout.transliterate("a:halant-zwj:"), "আ:halant-zwj:");

    EXPECT_EQ(layout.transliterate("k:halant::zwnj:"), "ক\u09CD\u200C");
    EXPECT_EQ(layout.transliterate("k:halant::zwj:"), "ক\u09CD\u200D");
    EXPECT_EQ(layout.transliterate(":zwnj:"), ":zwnj:");
    EXPECT_EQ(layout.transliterate(":zwj:"), ":zwj:");
    EXPECT_EQ(layout.transliterate("k:zwnj:"), "ক:zwnj:");
    EXPECT_EQ(layout.transliterate("k:zwj:"), "ক:zwj:");

    EXPECT_EQ(layout.transliterate("D:nukta:"), "\u09A1\u09BC");
    EXPECT_EQ(layout.transliterate(":nukta:"), ":nukta:");
    EXPECT_EQ(layout.transliterate("ka:nukta:"), "কা:nukta:");
    EXPECT_EQ(layout.transliterate("k:halant::nukta:"), "ক\u09CD:nukta:");
    EXPECT_EQ(layout.transliterate("R:nukta:"), "\u09A1\u09BC:nukta:");

    EXPECT_EQ(layout.transliterate(":danda:"), "\u0964");
    EXPECT_EQ(layout.transliterate(":ddanda:"), "\u0965");
}

TEST(PhoneticParserTest, MapsDigitsAndHonorsExplicitEscapes) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("0123456789"), "০১২৩৪৫৬৭৮৯");
    EXPECT_EQ(layout.transliterate("\\0"), "0");
    EXPECT_EQ(layout.transliterate("\\:"), ":");
    EXPECT_EQ(layout.transliterate("\\\\"), "\\");
    EXPECT_EQ(layout.transliterate("\\কা"), "কা");
}

TEST(PhoneticParserTest, DistinguishesPendingAndCommittedIncompleteControls) {
    const auto pending = suborno::layouts::phonetic::detail::parse_primary("ami:dan", false);
    EXPECT_TRUE(pending.pending);
    EXPECT_EQ(pending.text, "আমি");

    const auto committed = suborno::layouts::phonetic::detail::parse_primary("ami:dan", true);
    EXPECT_FALSE(committed.pending);
    EXPECT_EQ(committed.text, "আমি:dan");

    const auto escape_pending = suborno::layouts::phonetic::detail::parse_primary("ami\\", false);
    EXPECT_TRUE(escape_pending.pending);
    EXPECT_EQ(escape_pending.text, "আমি");

    EXPECT_THROW(suborno::layouts::phonetic::detail::transliterate_primary("\xF0\x80"),
                 std::invalid_argument);
}

TEST(PhoneticParserTest, EmitsExplicitConjunctSequencesAndNfcOutput) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("kt"), "ক\u09CDত");
    EXPECT_EQ(layout.transliterate("kya"), "ক\u09CDযা");
    EXPECT_EQ(layout.transliterate("ksh"), "ক\u09CDষ");
    EXPECT_EQ(layout.transliterate("Rh"), "\u09A2\u09BC");
    EXPECT_EQ(layout.transliterate("R"), "\u09A1\u09BC");
}

TEST(PhoneticParserTest, KeepsLegacyExamplesAsTokenDerivedOutput) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    EXPECT_EQ(layout.transliterate("bangla"), "বাংলা");
    EXPECT_EQ(layout.transliterate("bhasha"), "ভাশা");
    EXPECT_EQ(layout.transliterate("bhaSha"), "ভাষা");
    EXPECT_EQ(layout.transliterate("shikkha"), "শিক্ষা");
    EXPECT_EQ(layout.transliterate("bhalobasa"), "ভালোবাসা");
    EXPECT_EQ(layout.transliterate("bhalobasha"), "ভালোবাশা");
}

TEST(PhoneticParserTest, KeepsLegacyVectorsTokenDerivedWithoutWordExceptions) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("nodi"), "নোদি");
    EXPECT_EQ(layout.transliterate("Dhaka"), "ঢাকা");
    EXPECT_EQ(layout.transliterate("dhaka"), "ধাকা");
    EXPECT_EQ(layout.transliterate("shob"), "শোব");
    EXPECT_EQ(layout.transliterate("sôb"), "সব");
}
