#include "suborno/layouts/layout_input_session.hpp"

#include <gtest/gtest.h>

namespace {

class TestStatelessLayout final : public suborno::layout::StatelessInputLayout {
  public:
    suborno::layout::LayoutKind kind() const noexcept override {
        return suborno::layout::LayoutKind::fixed;
    }

    std::string transliterate(const std::string& input) const override {
        std::string output;
        for (const char character : input) {
            if (character == 'a') {
                output += "ক";
            } else if (character == 'b') {
                output += "খ";
            } else {
                output.push_back(character);
            }
        }
        return output;
    }

    std::vector<suborno::layout::Candidate> generate_candidates(
        const std::string& input) const override {
        const std::string output = transliterate(input);
        return {{
            .text = output,
            .normalized_text = output,
            .stable_id = "test-fixed:" + output,
            .source = suborno::layout::CandidateSource::layout,
            .score = 1.0,
            .rationale = "test fixed layout",
        }};
    }
};

TEST(LayoutInputSessionTest, BuildsPreeditFromTheCompleteKeySequence) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session.append_utf8("a"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session.append_utf8("b"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "ab");
    EXPECT_EQ(session.snapshot().preedit_utf8, "কখ");
}

TEST(LayoutInputSessionTest, BackspaceRemovesOneGraphemeAndRebuildsPreedit) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session.append_utf8("ab"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session.backspace(), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "a");
    EXPECT_EQ(session.snapshot().preedit_utf8, "ক");
}

TEST(LayoutInputSessionTest, SelectsAndCommitsTheActiveCandidate) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session.append_utf8("ab"), suborno::layout::InputStatus::updated);
    ASSERT_TRUE(session.select_candidate(0));
    EXPECT_EQ(session.commit().text_utf8, "কখ");
    EXPECT_EQ(session.snapshot().status, suborno::layout::InputStatus::empty);
}

TEST(LayoutInputSessionTest, RejectsInvalidUtf8WithoutMutatingTheSession) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session.append_utf8("\xFF"), suborno::layout::InputStatus::invalid_utf8);
    EXPECT_TRUE(session.snapshot().raw_input_utf8.empty());
}

TEST(LayoutInputSessionTest, AcceptsPhysicalKeysThroughTheSharedSessionContract) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(
                  {suborno::layout::PhysicalKey::letter_a, {.shift = true}}),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key(
                  {suborno::layout::PhysicalKey::semicolon, {.shift = true}}),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "A:");
}

TEST(LayoutInputSessionTest, DoesNotTurnShortcutChordsIntoCompositionText) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key(
                  {suborno::layout::PhysicalKey::letter_a, {.control = true}}),
              suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
}

TEST(LayoutInputSessionTest, EnforcesTheSharedInputLimit) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    const std::string input(suborno::layout::InputSession::kMaxInputCodePoints, 'a');
    ASSERT_EQ(session.append_utf8(input), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session.append_utf8("b"), suborno::layout::InputStatus::input_limit);
    EXPECT_EQ(session.snapshot().raw_input_utf8, input);
}

}  // namespace
