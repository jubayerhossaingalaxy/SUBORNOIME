#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layouts/layout_input_session.hpp"

#include <filesystem>
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

    EXPECT_EQ(session->append_utf8("a"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_utf8("b"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "ab");
    EXPECT_EQ(session->snapshot().preedit_utf8, "কখ");
}

TEST(LayoutInputSessionTest, BackspaceRemovesOneGraphemeAndRebuildsPreedit) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("ab"), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->backspace(), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "a");
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
}

TEST(LayoutInputSessionTest, SelectsAndCommitsTheActiveCandidate) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(session->append_utf8("ab"), suborno::layout::InputStatus::updated);
    ASSERT_TRUE(session->select_candidate(0));
    EXPECT_EQ(session->commit().text_utf8, "কখ");
    EXPECT_EQ(session->snapshot().status, suborno::layout::InputStatus::empty);
}

TEST(LayoutInputSessionTest, RejectsInvalidUtf8WithoutMutatingTheSession) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_utf8("\xFF"), suborno::layout::InputStatus::invalid_utf8);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
}

TEST(LayoutInputSessionTest, AcceptsPhysicalKeysThroughTheSharedSessionContract) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key({suborno::layout::PhysicalKey::letter_a, {.shift = true}}),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_key({suborno::layout::PhysicalKey::semicolon, {.shift = true}}),
              suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "A:");
}

TEST(LayoutInputSessionTest, DoesNotTurnShortcutChordsIntoCompositionText) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->append_key({suborno::layout::PhysicalKey::letter_a, {.control = true}}),
              suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
}

TEST(LayoutInputSessionTest, EnforcesTheSharedInputLimit) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    const std::string input(suborno::layout::InputSession::kMaxInputCodePoints, 'a');
    ASSERT_EQ(session->append_utf8(input), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->append_utf8("b"), suborno::layout::InputStatus::input_limit);
    EXPECT_EQ(session->snapshot().raw_input_utf8, input);
}

}  // namespace


TEST(LayoutInputSessionTest, SharedCandidateEngineAddsDictionaryMatchesForFixedLayouts) {
    const std::filesystem::path db_path =
        std::filesystem::temp_directory_path() / "suborno_layout_session_dictionary.sqlite";
    std::filesystem::remove(db_path);

    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        dictionary.add_entry("কখ", "কখ", 7);
        dictionary.add_entry("শব্দ", "ab", 5);

        TestStatelessLayout layout;
        auto session = layout.create_session(&dictionary, {.candidate_limit = 2});
        ASSERT_NE(session, nullptr);
        ASSERT_EQ(session->append_utf8("ab"), suborno::layout::InputStatus::updated);

        const auto snapshot = session->snapshot();
        ASSERT_EQ(snapshot.candidates.size(), 2U);
        EXPECT_EQ(snapshot.candidates[0].text, "কখ");
        EXPECT_EQ(snapshot.candidates[1].text, "শব্দ");
        EXPECT_EQ(snapshot.candidates[0].source, suborno::layout::CandidateSource::system_dictionary);
        EXPECT_EQ(snapshot.candidates[1].source, suborno::layout::CandidateSource::system_dictionary);
    }

    std::filesystem::remove(db_path);
}


TEST(LayoutInputSessionTest, UsesLogicalUnicodeTextFromActiveKeyboardLayout) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    suborno::layout::KeyEvent event{
        .key = suborno::layout::PhysicalKey::letter_a,
        .logical_text_utf8 = "ক",
    };

    EXPECT_EQ(session->append_key(event), suborno::layout::InputStatus::updated);
    EXPECT_EQ(session->snapshot().raw_input_utf8, "ক");
    EXPECT_EQ(session->snapshot().preedit_utf8, "ক");
}


TEST(LayoutInputSessionTest, DoesNotUseUsFallbackWhenLayoutTranslationReturnsNoText) {
    TestStatelessLayout layout;
    auto session = layout.create_session(nullptr, {});
    ASSERT_NE(session, nullptr);

    suborno::layout::KeyEvent event{
        .key = suborno::layout::PhysicalKey::letter_a,
        .logical_text_available = true,
    };

    EXPECT_EQ(session->append_key(event), suborno::layout::InputStatus::empty);
    EXPECT_TRUE(session->snapshot().raw_input_utf8.empty());
}
