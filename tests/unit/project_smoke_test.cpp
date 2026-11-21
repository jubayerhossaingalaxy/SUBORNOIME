#include <gtest/gtest.h>

#include "suborno/core/project.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

TEST(ProjectSmokeTest, ExposesInitialInfrastructureAbiVersion) {
    // Verify the ABI version function returns the expected value.
    // Using a variable to avoid C6326 (comparison of constants).
    const unsigned int abi_version = suborno::project_abi_version();
    EXPECT_EQ(abi_version, 1U);
}

TEST(ProjectSmokeTest, HandlesCoreBanglaTransliterationExamples) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    EXPECT_EQ(layout.transliterate("ami"), "আমি");
    EXPECT_EQ(layout.transliterate("bangla"), "বাংলা");
    EXPECT_EQ(layout.transliterate("bangladesh"), "বাংলাদেশ");
    EXPECT_EQ(layout.transliterate("bhasha"), "ভাশা");
    EXPECT_EQ(layout.transliterate("shikkha"), "শিক্ষা");
    EXPECT_EQ(layout.transliterate("bhalobasha"), "ভালোবাশা");
}
