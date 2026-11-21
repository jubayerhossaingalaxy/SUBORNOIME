#include <array>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "suborno/suborno.h"

TEST(CApiTest, CreatesGenericInputSessionAndCommitsBengaliText) {
    std::unique_ptr<SUBORNO_Engine, decltype(&suborno_engine_destroy)> engine(
        suborno_engine_create(), &suborno_engine_destroy);
    ASSERT_NE(engine, nullptr);

    std::unique_ptr<SUBORNO_Session, decltype(&suborno_session_destroy)> session(
        suborno_session_create(engine.get()), &suborno_session_destroy);
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(suborno_session_append_utf8(session.get(), "ami"), SUBORNO_STATUS_UPDATED);

    std::array<char, 64> preedit{};
    EXPECT_EQ(suborno_session_snapshot(session.get(), preedit.data(), preedit.size()),
              SUBORNO_STATUS_UPDATED);
    EXPECT_STREQ(preedit.data(), "আমি");

    char* committed_text = nullptr;
    std::size_t committed_size = 0;
    const int commit_status =
        suborno_session_commit(session.get(), &committed_text, &committed_size);
    std::unique_ptr<char, decltype(&suborno_string_free)> committed(
        committed_text, &suborno_string_free);

    ASSERT_EQ(commit_status, SUBORNO_STATUS_UPDATED);
    ASSERT_NE(committed, nullptr);
    EXPECT_EQ(std::string(committed.get(), committed_size), "আমি");
}
