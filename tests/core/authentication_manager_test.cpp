// tests/core/authentication_manager_test.cpp
#include "game_launcher/core/authentication_manager.h"
#include "game_launcher/core/user_settings.h" // For IUserSettings interface

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>   // For std::shared_ptr, std::unique_ptr
#include <optional>
#include <string>

// Define necessary actions and matchers from GMock
using ::testing::_;
using ::testing::Return;
using ::testing::Eq;
using ::testing::ByMove; // If needed for returning unique_ptr etc.
using ::testing::StrictMock; // Use StrictMock to catch unexpected calls

namespace game_launcher {
namespace core {

// --- Mock for IUserSettings ---
class MockUserSettings : public IUserSettings {
public:
    ~MockUserSettings() override = default;

    // Mock methods from IUserSettings that AuthenticationManager uses
    MOCK_METHOD(absl::Status, LoadSettings, (), (override));
    MOCK_METHOD(absl::Status, SaveSettings, (), (override));
    MOCK_METHOD(bool, IsDirty, (), (const, override));

    // Methods directly used by BasicAuthenticationManager placeholder
    // Corrected names and key type (std::string_view)
    MOCK_METHOD(absl::StatusOr<std::string>, GetString, (std::string_view key), (const, override));
    MOCK_METHOD(absl::Status, SetString, (std::string_view key, std::string_view value), (override));

    // Mock other Get/Set methods correctly
    MOCK_METHOD(absl::StatusOr<int>, GetInt, (std::string_view key), (const, override));
    MOCK_METHOD(absl::Status, SetInt, (std::string_view key, int value), (override));
    MOCK_METHOD(absl::StatusOr<bool>, GetBool, (std::string_view key), (const, override));
    MOCK_METHOD(absl::Status, SetBool, (std::string_view key, bool value), (override));

    // Mock remaining pure virtual methods from IUserSettings
    MOCK_METHOD(bool, HasKey, (std::string_view key), (const, override));
    MOCK_METHOD(absl::Status, RemoveKey, (std::string_view key), (override));
    MOCK_METHOD(absl::Status, Clear, (), (override));

};

// --- Test Fixture ---
class AuthenticationManagerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockUserSettings> mock_settings_;
    std::unique_ptr<IAuthenticationManager> auth_manager_;

    void SetUp() override {
        // Restore StrictMock
        mock_settings_ = std::make_shared<StrictMock<MockUserSettings>>();

        // --- Default Expectations for LoadSession ---
        // Expect calls to GetStringValue during LoadSession in CreateAuthenticationManager
        // Simulate no existing session by default
        // Corrected method name GetString
        EXPECT_CALL(*mock_settings_, GetString(Eq("auth.session_token")))
            .WillOnce(Return(absl::NotFoundError("Token not found")));
        // We might need expectations for user_id, username, email as well if token is found,
        // but for the default 'no session' case, only the token check matters initially.

        // --- Instantiate the manager ---
        auth_manager_ = CreateAuthenticationManager(mock_settings_);
        ASSERT_NE(auth_manager_, nullptr);
    }

    void TearDown() override {
        auth_manager_.reset();
        mock_settings_.reset();
    }
};

// --- Test Cases ---

TEST_F(AuthenticationManagerTest, InitialStateIsLoggedOut) {
    ASSERT_NE(auth_manager_, nullptr);
    EXPECT_EQ(auth_manager_->GetAuthStatus(), AuthStatus::kLoggedOut);
    auto profile_status = auth_manager_->GetCurrentUserProfile();
    EXPECT_FALSE(profile_status.ok());
    EXPECT_EQ(profile_status.status().code(), absl::StatusCode::kNotFound);
}

TEST_F(AuthenticationManagerTest, LoginSuccess) {
    ASSERT_NE(auth_manager_, nullptr);

    // Expect UserSettings calls during successful login
    // Corrected method name SetString
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.session_token"), Eq("dummy_token_12345")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.user_id"), Eq("user123")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.username"), Eq("testuser")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.email"), Eq("testuser@example.com")))
        .WillOnce(Return(absl::OkStatus()));

    // Perform login
    absl::Status login_status = auth_manager_->Login("testuser", "password");
    EXPECT_TRUE(login_status.ok()) << login_status;

    // Verify state after login
    EXPECT_EQ(auth_manager_->GetAuthStatus(), AuthStatus::kLoggedIn);
    auto profile_status = auth_manager_->GetCurrentUserProfile();
    ASSERT_TRUE(profile_status.ok()) << profile_status.status();
    EXPECT_EQ(profile_status.value().user_id, "user123");
    EXPECT_EQ(profile_status.value().username, "testuser");
    EXPECT_EQ(profile_status.value().email, "testuser@example.com");
}

TEST_F(AuthenticationManagerTest, LoginFailureInvalidCredentials) {
    ASSERT_NE(auth_manager_, nullptr);
    // No calls to SetStringValue expected on failure

    // Perform login with bad credentials
    absl::Status login_status = auth_manager_->Login("wronguser", "wrongpassword");
    EXPECT_FALSE(login_status.ok());
    EXPECT_EQ(login_status.code(), absl::StatusCode::kUnauthenticated);

    // Verify state remains logged out
    EXPECT_EQ(auth_manager_->GetAuthStatus(), AuthStatus::kLoggedOut);
    auto profile_status = auth_manager_->GetCurrentUserProfile();
    EXPECT_FALSE(profile_status.ok());
    EXPECT_EQ(profile_status.status().code(), absl::StatusCode::kNotFound);
}

TEST_F(AuthenticationManagerTest, LoginFailureEmptyCredentials) {
    ASSERT_NE(auth_manager_, nullptr);
    // No calls to SetStringValue expected on failure

    // Test empty username
    absl::Status login_status_empty_user = auth_manager_->Login("", "password");
    EXPECT_FALSE(login_status_empty_user.ok());
    EXPECT_EQ(login_status_empty_user.code(), absl::StatusCode::kInvalidArgument);
    EXPECT_EQ(auth_manager_->GetAuthStatus(), AuthStatus::kLoggedOut); // Status unchanged

    // Test empty password
    absl::Status login_status_empty_pass = auth_manager_->Login("testuser", "");
    EXPECT_FALSE(login_status_empty_pass.ok());
    EXPECT_EQ(login_status_empty_pass.code(), absl::StatusCode::kInvalidArgument);
    EXPECT_EQ(auth_manager_->GetAuthStatus(), AuthStatus::kLoggedOut); // Status unchanged
}

TEST_F(AuthenticationManagerTest, LogoutSuccess) {
    // First, successfully login to establish a session
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.session_token"), _)).WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.user_id"), _)).WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.username"), _)).WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.email"), _)).WillOnce(Return(absl::OkStatus()));
    ASSERT_TRUE(auth_manager_->Login("testuser", "password").ok());
    ASSERT_EQ(auth_manager_->GetAuthStatus(), AuthStatus::kLoggedIn);

    // Expect calls to SetString with empty values during logout
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.session_token"), Eq("")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.user_id"), Eq("")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.username"), Eq("")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*mock_settings_, SetString(Eq("auth.email"), Eq("")))
        .WillOnce(Return(absl::OkStatus()));

    // Perform logout
    absl::Status logout_status = auth_manager_->Logout();
    EXPECT_TRUE(logout_status.ok()) << logout_status;

    // Verify state after logout
    EXPECT_EQ(auth_manager_->GetAuthStatus(), AuthStatus::kLoggedOut);
    auto profile_status = auth_manager_->GetCurrentUserProfile();
    EXPECT_FALSE(profile_status.ok());
    EXPECT_EQ(profile_status.status().code(), absl::StatusCode::kNotFound);
}

TEST_F(AuthenticationManagerTest, LoadSessionSuccess) {
    // Need a separate fixture setup or explicit calls for this test
    // Create a new mock and manager instance for isolated testing
    // Restore StrictMock
    auto specific_mock_settings = std::make_shared<StrictMock<MockUserSettings>>();
    
    // Setup expectations for finding a valid session (using the correct dummy token)
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.session_token")))
        .WillOnce(Return("dummy_token_12345")); // Expect the valid token
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.user_id")))
        .WillOnce(Return("restored_user"));
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.username")))
        .WillOnce(Return("Restored User"));
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.email")))
        .WillOnce(Return("restored@example.com"));

    // Instantiate the manager, which calls LoadSession internally
    auto manager = CreateAuthenticationManager(specific_mock_settings);
    ASSERT_NE(manager, nullptr);

    // Verify state after load
    EXPECT_EQ(manager->GetAuthStatus(), AuthStatus::kLoggedIn);
    auto profile_status = manager->GetCurrentUserProfile();
    ASSERT_TRUE(profile_status.ok()) << profile_status.status();
    EXPECT_EQ(profile_status.value().user_id, "restored_user");
    EXPECT_EQ(profile_status.value().username, "Restored User");
    EXPECT_EQ(profile_status.value().email, "restored@example.com");
}

TEST_F(AuthenticationManagerTest, LoadSessionFailureNoToken) {
    // The default SetUp already configures this scenario
    ASSERT_NE(auth_manager_, nullptr);
    // Verify state (should match InitialStateIsLoggedOut)
    EXPECT_EQ(auth_manager_->GetAuthStatus(), AuthStatus::kLoggedOut);
    auto profile_status = auth_manager_->GetCurrentUserProfile();
    EXPECT_FALSE(profile_status.ok());
    EXPECT_EQ(profile_status.status().code(), absl::StatusCode::kNotFound);
}

TEST_F(AuthenticationManagerTest, LoadSessionFailureIncompleteData) {
    // Create a new mock and manager instance for isolated testing
    // Restore StrictMock
    auto specific_mock_settings = std::make_shared<StrictMock<MockUserSettings>>();
    
    // Setup expectations: Valid Token found, but user ID missing
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.session_token")))
        .WillOnce(Return("dummy_token_12345")); // Expect the valid token
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.user_id")))
        .WillOnce(Return(absl::NotFoundError("User ID not found")));
    // Username and email calls might still happen but won't lead to login
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.username")))
        .WillOnce(Return("Restored User")); 
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.email")))
        .WillOnce(Return("restored@example.com")); 

    // Instantiate the manager
    auto manager = CreateAuthenticationManager(specific_mock_settings);
    ASSERT_NE(manager, nullptr);

    // Verify state remains LoggedOut
    EXPECT_EQ(manager->GetAuthStatus(), AuthStatus::kLoggedOut);
    auto profile_status = manager->GetCurrentUserProfile();
    EXPECT_FALSE(profile_status.ok());
    EXPECT_EQ(profile_status.status().code(), absl::StatusCode::kNotFound);
}

TEST_F(AuthenticationManagerTest, LoadSessionFailureInvalidToken) {
    // Create a new mock and manager instance for isolated testing
    // Restore StrictMock
    auto specific_mock_settings = std::make_shared<StrictMock<MockUserSettings>>();
    
    // Setup expectations: An invalid token is found
    EXPECT_CALL(*specific_mock_settings, GetString(Eq("auth.session_token")))
        .WillOnce(Return("invalid_dummy_token_67890")); // Return an *incorrect* token
         
    // Expect that finding an invalid token triggers clearing of stale data
    EXPECT_CALL(*specific_mock_settings, SetString(Eq("auth.session_token"), Eq("")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*specific_mock_settings, SetString(Eq("auth.user_id"), Eq("")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*specific_mock_settings, SetString(Eq("auth.username"), Eq("")))
        .WillOnce(Return(absl::OkStatus()));
    EXPECT_CALL(*specific_mock_settings, SetString(Eq("auth.email"), Eq("")))
        .WillOnce(Return(absl::OkStatus()));

    // Instantiate the manager, which calls LoadSession internally
    auto manager = CreateAuthenticationManager(specific_mock_settings);
    ASSERT_NE(manager, nullptr);

    // Verify state remains LoggedOut
    EXPECT_EQ(manager->GetAuthStatus(), AuthStatus::kLoggedOut);
    auto profile_status = manager->GetCurrentUserProfile();
    EXPECT_FALSE(profile_status.ok());
    EXPECT_EQ(profile_status.status().code(), absl::StatusCode::kNotFound);
}

} // namespace core
} // namespace game_launcher
