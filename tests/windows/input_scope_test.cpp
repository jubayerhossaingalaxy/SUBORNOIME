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
