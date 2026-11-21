#include <filesystem>
#include <gtest/gtest.h>
#include <string>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/layouts/phonetic/phonetic_input_session.hpp"
#include "suborno/layouts/phonetic/suborno_phonetic_layout.hpp"

using suborno::layouts::InputStatus;
using suborno::layouts::PhoneticInputSession;

TEST(PhoneticInputSessionTest, LearningIsDisabledByDefaultForPublicSessionConstruction) {
    const std::filesystem::path db_path =
        std::filesystem::temp_directory_path() / "suborno_phonetic_session_default_learning.sqlite";
    std::filesystem::remove(db_path);

    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        suborno::layouts::SubornoPhoneticLayout layout;
        PhoneticInputSession session(layout, dictionary);
        ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);

        EXPECT_EQ(session.commit().text_utf8, "আমি");
        EXPECT_EQ(dictionary.get_user_frequency("আমি"), 0);
        EXPECT_TRUE(dictionary.list_user_words().empty());
    }

    std::filesystem::remove(db_path);
}

TEST(PhoneticInputSessionTest, DisablingLearningDuringCompositionPreventsDictionaryWrites) {
    const std::filesystem::path db_path =
        std::filesystem::temp_directory_path() / "suborno_phonetic_session_learning_opt_out.sqlite";
    std::filesystem::remove(db_path);

    {
        suborno::dictionary::Dictionary dictionary(db_path.string());
        suborno::layouts::SubornoPhoneticLayout layout;
        PhoneticInputSession session(layout, dictionary, {.learning_enabled = true});
        ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);

        session.set_learning_enabled(false);
        EXPECT_EQ(session.commit().text_utf8, "আমি");
        EXPECT_EQ(dictionary.get_user_frequency("আমি"), 0);
        EXPECT_TRUE(dictionary.list_user_words().empty());
    }

    std::filesystem::remove(db_path);
}

TEST(PhoneticInputSessionTest, AppendsValidInputAndCommitsPrimaryOutput) {
    PhoneticInputSession session;

    EXPECT_EQ(session.append_utf8("a"), InputStatus::updated);
    EXPECT_EQ(session.append_utf8("mi"), InputStatus::updated);
    const auto snapshot = session.snapshot();
    EXPECT_EQ(snapshot.raw_input_utf8, "ami");
    EXPECT_EQ(snapshot.preedit_utf8, "আমি");
    ASSERT_EQ(snapshot.candidates.size(), 1U);

    const auto committed = session.commit();
    EXPECT_EQ(committed.status, InputStatus::updated);
    EXPECT_EQ(committed.text_utf8, "আমি");
    EXPECT_EQ(session.snapshot().status, InputStatus::empty);
}

TEST(PhoneticInputSessionTest, RejectsInvalidUtf8AndLimitOverflowAtomically) {
    PhoneticInputSession session;

    ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);
    const auto before = session.snapshot();
    EXPECT_EQ(session.append_utf8("\xF0\x80"), InputStatus::invalid_utf8);
    EXPECT_EQ(session.snapshot().raw_input_utf8, before.raw_input_utf8);
    EXPECT_EQ(session.append_utf8(std::string(PhoneticInputSession::kMaxInputCodePoints, 'a')),
              InputStatus::input_limit);
    EXPECT_EQ(session.snapshot().raw_input_utf8, before.raw_input_utf8);
}

TEST(PhoneticInputSessionTest, EnforcesTheScalarLimitAndAllowsExactLimit) {
    PhoneticInputSession session;
    const std::string maximum(PhoneticInputSession::kMaxInputCodePoints, 'a');

    EXPECT_EQ(session.append_utf8(maximum), InputStatus::updated);
    EXPECT_EQ(session.snapshot().raw_input_utf8.size(), maximum.size());
    EXPECT_EQ(session.append_utf8("a"), InputStatus::input_limit);
    EXPECT_EQ(session.snapshot().raw_input_utf8, maximum);

    session.reset();
    std::string bengali_maximum;
    for (std::size_t index = 0; index < PhoneticInputSession::kMaxInputCodePoints; ++index) {
        bengali_maximum.append("ক");
    }
    EXPECT_EQ(session.append_utf8(bengali_maximum), InputStatus::literal_passthrough);
    EXPECT_EQ(session.append_utf8("ক"), InputStatus::input_limit);
}

TEST(PhoneticInputSessionTest, BackspaceRemovesOneExtendedGraphemeCluster) {
    PhoneticInputSession session;

    ASSERT_EQ(session.append_utf8("কা"), InputStatus::literal_passthrough);
    EXPECT_EQ(session.backspace(), InputStatus::empty);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "");
    EXPECT_EQ(session.backspace(), InputStatus::empty);
}

TEST(PhoneticInputSessionTest, BackspaceKeepsEmojiFlagAndCombiningGraphemesAtomic) {
    PhoneticInputSession session;

    for (const std::string& input : {
             std::string("?\U0001F469\u200D\U0001F4BB"),
             std::string("?\U0001F1E7\U0001F1E9"),
             std::string("?e\u0301"),
         }) {
        session.reset();
        ASSERT_NE(session.append_utf8(input), InputStatus::invalid_utf8);
        EXPECT_EQ(session.backspace(), InputStatus::literal_passthrough);
        EXPECT_EQ(session.snapshot().raw_input_utf8, "?");
    }
}

TEST(PhoneticInputSessionTest, KeepsIncompleteControlPendingUntilCommit) {
    PhoneticInputSession session;

    EXPECT_EQ(session.append_utf8("ami:dan"), InputStatus::pending);
    const auto snapshot = session.snapshot();
    EXPECT_EQ(snapshot.status, InputStatus::pending);
    EXPECT_EQ(snapshot.preedit_utf8, "আমি");
    EXPECT_TRUE(snapshot.candidates.empty());

    const auto committed = session.commit();
    EXPECT_EQ(committed.text_utf8, "আমি:dan");
    EXPECT_EQ(session.snapshot().status, InputStatus::empty);
}

TEST(PhoneticInputSessionTest, ReportsEscapedAndInvalidControlLiteralPassThrough) {
    PhoneticInputSession session;

    EXPECT_EQ(session.append_utf8("\\0"), InputStatus::literal_passthrough);
    EXPECT_EQ(session.snapshot().preedit_utf8, "0");
    session.reset();

    EXPECT_EQ(session.append_utf8(":nukta:"), InputStatus::literal_passthrough);
    EXPECT_EQ(session.snapshot().status, InputStatus::literal_passthrough);
    EXPECT_EQ(session.snapshot().preedit_utf8, ":nukta:");
}

TEST(PhoneticInputSessionTest, InvalidCandidateIndexDoesNotClearComposition) {
    PhoneticInputSession session;

    ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);
    const auto invalid = session.commit_candidate(1);
    EXPECT_EQ(invalid.status, InputStatus::candidate_out_of_range);
    EXPECT_EQ(session.snapshot().raw_input_utf8, "ami");

    const auto committed = session.commit_candidate(0);
    EXPECT_EQ(committed.text_utf8, "আমি");
    EXPECT_EQ(committed.status, InputStatus::updated);
}

TEST(PhoneticInputSessionTest, ResetDropsCompositionWithoutCommitting) {
    PhoneticInputSession session;

    ASSERT_EQ(session.append_utf8("ami"), InputStatus::updated);
    session.reset();
    EXPECT_EQ(session.snapshot().status, InputStatus::empty);
}
