#include <gtest/gtest.h>

#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

TEST(LinguisticRegressionTest, CoreBanglaVectorsRemainStable) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    const struct {
        const char* input;
        const char* output;
    } vectors[] = {
        {"ami", "আমি"},
        {"bangla", "বাংলা"},
        {"bhasha", "ভাশা"},
        {"bhaSha", "ভাষা"},
        {"shikkha", "শিক্ষা"},
        {"bhalobasa", "ভালোবাসা"},
        {"nodi", "নোদি"},
        {"Dhaka", "ঢাকা"},
        {"dhaka", "ধাকা"},
        {"shob", "শোব"},
        {"sôb", "সব"},
        {"kt", "ক\u09CDত"},
        {"kya", "ক\u09CDযা"},
        {"ksh", "ক\u09CDষ"},
        {"Rh", "\u09A2\u09BC"},
    };

    for (const auto& vector : vectors) {
        EXPECT_EQ(layout.transliterate(vector.input), vector.output)
            << "phonetic input: " << vector.input;
    }
}
