#include <gtest/gtest.h>

#include "suborno/platform/windows/tsf/input_scope.hpp"

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
