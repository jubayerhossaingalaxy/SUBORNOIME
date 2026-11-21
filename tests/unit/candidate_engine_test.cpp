#include "suborno/candidates/candidate_engine.hpp"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layout-api/input_layout.hpp"

namespace {

class TestFixedLayout final : public suborno::layout::StatelessInputLayout {
  public:
    suborno::layout::LayoutKind kind() const noexcept override {
        return suborno::layout::LayoutKind::fixed;
    }

    std::string transliterate(const std::string& input) const override {
        return input == "ab" ? "বাংলা" : input;
    }

    std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        (void)input;
        return {
            {"বাংলা",
             "বাংলা",
             "fixed:বাংলা",
             suborno::layout::CandidateSource::layout,
             1.0,
             "fixed"},
            {"বই", "বই", "fixed:বই", suborno::layout::CandidateSource::layout, 0.5, "fixed"},
        };
    }
};

std::string temporary_database_path(const char* filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

}  // namespace

TEST(CandidateEngineTest, SupportsFixedLayoutsAndMergesDictionaryCandidates) {
    const std::string db_path =
        temporary_database_path("suborno_candidate_engine_fixed_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);
    dictionary.add_entry("ভাষা", "ab", 5);

    const TestFixedLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, dictionary);
    const auto candidates = engine.make_candidates("ab", 3);

    ASSERT_EQ(candidates.size(), 3U);
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[0].source, suborno::layout::CandidateSource::system_dictionary);
    EXPECT_EQ(candidates[1].text, "ভাষা");
    EXPECT_EQ(candidates[1].source, suborno::layout::CandidateSource::system_dictionary);
    EXPECT_EQ(candidates[2].text, "বই");
    EXPECT_EQ(candidates[2].source, suborno::layout::CandidateSource::layout);

    std::remove(db_path.c_str());
}

TEST(CandidateEngineTest, AppliesTheCandidateLimitAndSkipsDuplicateLayoutOutput) {
    const std::string db_path =
        temporary_database_path("suborno_candidate_engine_limit_test.sqlite");
    std::remove(db_path.c_str());

    suborno::dictionary::Dictionary dictionary(db_path);
    dictionary.add_entry("বাংলা", "বাংলা", 10);

    const TestFixedLayout layout;
    const suborno::candidates::CandidateEngine engine(layout, &dictionary);
    const auto candidates = engine.make_candidates("বাংলা", 2);

    ASSERT_EQ(candidates.size(), 2U);
    EXPECT_EQ(candidates[0].text, "বাংলা");
    EXPECT_EQ(candidates[1].text, "বই");
    EXPECT_TRUE(engine.make_candidates("বাংলা", 0).empty());

    std::remove(db_path.c_str());
}
