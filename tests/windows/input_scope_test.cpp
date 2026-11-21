#include "suborno/platform/windows/tsf/input_scope.hpp"

#include <gtest/gtest.h>

TEST(InputScopeTest, ClassifiesPasswordAndPrivateScopesAsSensitive) {
    EXPECT_TRUE(suborno::platform::windows::is_sensitive_input_scope_type(IS_PASSWORD));
    EXPECT_TRUE(suborno::platform::windows::is_sensitive_input_scope_type(IS_PRIVATE));
    EXPECT_TRUE(suborno::platform::windows::is_sensitive_input_scope_type(IS_NUMERIC_PASSWORD));
    EXPECT_TRUE(suborno::platform::windows::is_sensitive_input_scope_type(IS_NUMERIC_PIN));
    EXPECT_TRUE(suborno::platform::windows::is_sensitive_input_scope_type(IS_ALPHANUMERIC_PIN));
    EXPECT_TRUE(suborno::platform::windows::is_sensitive_input_scope_type(IS_ALPHANUMERIC_PIN_SET));
}

TEST(InputScopeTest, DoesNotTreatOrdinaryTextScopesAsSensitive) {
    EXPECT_FALSE(suborno::platform::windows::is_sensitive_input_scope_type(IS_DEFAULT));
    EXPECT_FALSE(suborno::platform::windows::is_sensitive_input_scope_type(IS_TEXT));
    EXPECT_FALSE(suborno::platform::windows::is_sensitive_input_scope_type(IS_URL));
    EXPECT_FALSE(
        suborno::platform::windows::is_sensitive_input_scope_type(IS_EMAIL_SMTPEMAILADDRESS));
}

TEST(InputScopeTest, CapturesTextOnlyWhenTheScopeWasReadAndIsNotSensitive) {
    using suborno::platform::windows::may_capture_text_from_input_scope;

    EXPECT_TRUE(may_capture_text_from_input_scope(S_OK, false));
    EXPECT_FALSE(may_capture_text_from_input_scope(S_OK, true));
    EXPECT_FALSE(may_capture_text_from_input_scope(S_FALSE, false));
    EXPECT_FALSE(may_capture_text_from_input_scope(E_FAIL, false));
}

TEST(InputScopeTest, LifecycleCommitsRequireAFreshSafeScopeAndNoPreviousDenial) {
    using suborno::platform::windows::may_commit_composition_from_input_scope;

    EXPECT_TRUE(may_commit_composition_from_input_scope(S_OK, false, false));
    EXPECT_FALSE(may_commit_composition_from_input_scope(S_OK, true, false));
    EXPECT_FALSE(may_commit_composition_from_input_scope(E_FAIL, false, false));
    EXPECT_FALSE(may_commit_composition_from_input_scope(S_FALSE, false, false));
    EXPECT_FALSE(may_commit_composition_from_input_scope(S_OK, false, true));
}

TEST(InputScopeTest, CancelsExistingCompositionWhenCaptureMustFailClosed) {
    using suborno::platform::windows::InputScopeCaptureDecision;
    using suborno::platform::windows::decide_input_scope_capture;

    EXPECT_EQ(decide_input_scope_capture(S_OK, false, true),
              InputScopeCaptureDecision::allow_capture);
    EXPECT_EQ(decide_input_scope_capture(S_OK, true, true),
              InputScopeCaptureDecision::cancel_composition);
    EXPECT_EQ(decide_input_scope_capture(E_FAIL, false, true),
              InputScopeCaptureDecision::cancel_composition);
    EXPECT_EQ(decide_input_scope_capture(S_OK, true, false),
              InputScopeCaptureDecision::pass_through);
    EXPECT_EQ(decide_input_scope_capture(E_FAIL, false, false),
              InputScopeCaptureDecision::pass_through);
}
