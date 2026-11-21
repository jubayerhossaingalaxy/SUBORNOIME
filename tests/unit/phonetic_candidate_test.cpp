#include <cmath>
#include <gtest/gtest.h>
#include <vector>

#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

namespace {

void expect_candidate_texts(const std::vector<suborno::layout::Candidate>& candidates,
                            const std::vector<std::string>& expected) {
    ASSERT_EQ(candidates.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(candidates[index].text, expected[index]) << "candidate rank " << index;
        EXPECT_DOUBLE_EQ(candidates[index].score, 1.0 / static_cast<double>(index + 1));
    }
}

}  // namespace

TEST(PhoneticCandidateTest, OrdersChAndShWholeTokenAndSplitAlternatives) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    expect_candidate_texts(layout.generate_candidates("ch"), {"ছ", "চ\u09CDহ", "চ"});
    expect_candidate_texts(layout.generate_candidates("sh"), {"শ", "ষ", "স\u09CDহ"});
}

TEST(PhoneticCandidateTest, GeneratesLongestMatchAndVowelAlternatives) {
    const suborno::layouts::SubornoPhoneticLayout layout;

    expect_candidate_texts(layout.generate_candidates("chh"), {"ছ", "ছ\u09CDহ"});
    expect_candidate_texts(layout.generate_candidates("ksh"), {"ক\u09CDষ", "ক\u09CDশ"});
    expect_candidate_texts(layout.generate_candidates("kkh"), {"ক\u09CDষ", "ক\u09CDখ"});
    expect_candidate_texts(layout.generate_candidates("ai"), {"ঐ", "আই"});
    expect_candidate_texts(layout.generate_candidates("oi"), {"ঐ", "ওই"});
    expect_candidate_texts(layout.generate_candidates("au"), {"ঔ", "আউ"});
    expect_candidate_texts(layout.generate_candidates("ou"), {"ঔ", "ওউ"});
    expect_candidate_texts(layout.generate_candidates("kai"), {"ক\u09C8", "কা\u0987"});
    expect_candidate_texts(layout.generate_candidates("koi"), {"ক\u09C8", "কো\u0987"});
    expect_candidate_texts(layout.generate_candidates("kau"), {"ক\u09CC", "কা\u0989"});
    expect_candidate_texts(layout.generate_candidates("kou"), {"ক\u09CC", "কো\u0989"});
    expect_candidate_texts(layout.generate_candidates("aoi"), {"আঐ", "আওই"});
}

TEST(PhoneticCandidateTest, GeneratesAllAmbiguityCombinationsInStableOrder) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    const auto candidates = layout.generate_candidates("ch sh");

    expect_candidate_texts(candidates,
                           {"ছ শ",
                            "চ\u09CDহ শ",
                            "ছ ষ",
                            "চ\u09CDহ ষ",
                            "চ শ",
                            "ছ স\u09CDহ",
                            "চ\u09CDহ স\u09CDহ",
                            "চ ষ",
                            "চ স\u09CDহ"});
}

TEST(PhoneticCandidateTest, AppliesTheSixteenCandidateCapAndDeduplicatesOutputs) {
    const suborno::layouts::SubornoPhoneticLayout layout;
    const auto candidates = layout.generate_candidates("chchchchchchchchchch");

    ASSERT_EQ(candidates.size(), 16U);
    EXPECT_EQ(candidates.front().text, layout.transliterate("chchchchchchchchchch"));
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        EXPECT_DOUBLE_EQ(candidates[index].score, 1.0 / static_cast<double>(index + 1));
    }

    const auto duplicate_check = layout.generate_candidates("x");
    ASSERT_EQ(duplicate_check.size(), 1U);
    EXPECT_EQ(duplicate_check.front().text, "ক্ষ");
}
