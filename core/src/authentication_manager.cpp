// core/src/authentication_manager.cpp

#include "game_launcher/core/authentication_manager.h"
#include "game_launcher/core/user_settings.h"
#include "game_launcher/core/app_settings.h" // Include AppSettings definition
#include "game_launcher/core/auth_status.h" // Include AuthStatus enum

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/memory/memory.h" // For absl::make_unique
#include "nlohmann/json.hpp"      // For JSON serialization
#include "include/base/cef_logging.h" // Correct CEF logging include (full path)

#include <optional>
#include <memory>
#include <string>

namespace game_launcher {
namespace core {

// Allow nlohmann::json to serialize/deserialize UserProfile
// Ensure this matches the definition in authentication_manager.h
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserProfile, user_id, username, email)

// Concrete implementation of the Authentication Manager
class AuthenticationManager : public IAuthenticationManager {
public:
    // Constructor requires UserSettings dependency
    explicit AuthenticationManager(std::shared_ptr<IUserSettings> user_settings)
        : user_settings_(std::move(user_settings)),
          current_status_(AuthStatus::kLoggedOut) // Start logged out
    {
        LoadSessionFromSettings();
    }

    absl::Status Login(const std::string& username, const std::string& password) override {
        // --- Placeholder Implementation ---
        // In a real scenario, this would involve:
        // 1. Sending credentials to a backend API (e.g., via an HTTP client).
        // 2. Receiving session tokens (e.g., JWT) on success.
        // 3. Storing tokens securely using user_settings_.
        // 4. Fetching the user profile.

        // Simulate login success for a specific user/pass
        if (username == "testuser" && password == "password") {
            current_status_ = AuthStatus::kLoggedIn;
            current_profile_ = UserProfile{
                .user_id = "user-123",
                .username = username,
                .email = "testuser@example.com"
            };
            // Store profile in user_settings_
            if (!user_settings_) {
                // No need to save if settings aren't available
                // Log appropriately if this is unexpected
                LOG(WARNING) << "UserSettings not available during login, session not saved.";
                return absl::OkStatus(); // Login succeeded locally, but couldn't save
            }
            if (!SaveSessionToSettings()) {
                LOG(ERROR) << "Failed to save session data after login.";
                // Decide if this is a critical error - perhaps logout again?
            }
            LOG(INFO) << "Simulated login successful for user: " << username;
            return absl::OkStatus();
        } else {
            LOG(WARNING) << "Simulated login failed for user: " << username;
            // Clear session if login fails
            current_status_ = AuthStatus::kLoggedOut;
            current_profile_.reset();
            if (user_settings_) {
                SaveSessionToSettings(); // Save the logged-out state (clears profile)
            }
            return absl::UnauthenticatedError(absl::StrCat("Invalid credentials for user: ", username));
        }
    }

    absl::Status Logout() override {
        // --- Placeholder Implementation ---
        // In a real scenario, this would involve:
        // 1. Calling a backend API to invalidate the session (optional).
        // 2. Clearing local session tokens via user_settings_.

        LOG(INFO) << "User logging out: " << (current_profile_ ? current_profile_->username : "N/A");
        current_status_ = AuthStatus::kLoggedOut;
        current_profile_.reset();

        // Save the logged-out state (which clears the profile in settings)
        if (user_settings_) {
            if (!SaveSessionToSettings()) {
                LOG(ERROR) << "Failed to clear session data from settings during logout.";
                // Non-critical for logout itself, but indicates a settings issue.
            }
        }
        return absl::OkStatus();
    }

    AuthStatus GetAuthStatus() const override {
        // TODO: Add logic to check token validity/expiration if applicable
        return current_status_;
    }

    absl::StatusOr<UserProfile> GetCurrentUserProfile() const override {
        if (current_status_ == AuthStatus::kLoggedIn && current_profile_) {
            // Explicitly return UserProfile type, which implicitly converts to StatusOr<UserProfile>
            return UserProfile{*current_profile_};
        }
        return absl::NotFoundError("User is not currently logged in or profile is unavailable.");
    }

private:
    // Load session state from user settings
    void LoadSessionFromSettings() {
        if (!user_settings_) {
            LOG(WARNING) << "UserSettings not available, cannot load session.";
            return;
        }

        AppSettings settings = user_settings_->GetAppSettings();

        if (settings.user_profile_json.has_value() && !settings.user_profile_json->empty()) {
            try {
                nlohmann::json profile_json = nlohmann::json::parse(settings.user_profile_json.value());
                current_profile_ = profile_json.get<UserProfile>();
                current_status_ = AuthStatus::kLoggedIn;
                LOG(INFO) << "Loaded existing session for user: " << current_profile_->username;
            } catch (const nlohmann::json::exception& e) {
                LOG(ERROR) << "Failed to parse stored user profile JSON: " << e.what();
                // Clear potentially corrupted data by saving the current (logged out) state
                current_status_ = AuthStatus::kLoggedOut;
                current_profile_.reset();
                SaveSessionToSettings(); // Attempt to save cleared state
            }
        } else {
             // No profile found or empty, ensure logged out state
             LOG(INFO) << "No existing user session found in settings.";
             current_status_ = AuthStatus::kLoggedOut;
             current_profile_.reset();
        }
    }

    // Save current session state to user settings
    bool SaveSessionToSettings() {
         if (!user_settings_) {
            LOG(ERROR) << "UserSettings not available, cannot save session.";
            return false;
        }

        AppSettings settings = user_settings_->GetAppSettings(); // Get current settings

        if (current_status_ == AuthStatus::kLoggedIn && current_profile_) {
             try {
                nlohmann::json profile_json(*current_profile_); // Explicitly construct json from UserProfile
                settings.user_profile_json = profile_json.dump(); // Set the profile JSON in settings
            } catch (const nlohmann::json::exception& e) {
                LOG(ERROR) << "Failed to serialize user profile to JSON: " << e.what();
                return false; // Don't attempt to save if serialization failed
            }
        } else {
            // If not logged in, ensure the profile field is cleared
            settings.user_profile_json = std::nullopt;
        }

        // Save the potentially modified settings object
        absl::Status set_status = user_settings_->SetAppSettings(settings);
        if (!set_status.ok()) {
            LOG(ERROR) << "Failed to save user settings: " << set_status;
            return false;
        }

        LOG(INFO) << "User session state saved successfully.";
        return true;
    }

    std::shared_ptr<IUserSettings> user_settings_;
    AuthStatus current_status_;
    std::optional<UserProfile> current_profile_;
    // TODO: Add member for storing session token (securely if possible)
};

// Factory function implementation
std::unique_ptr<IAuthenticationManager> CreateAuthenticationManager(std::shared_ptr<IUserSettings> user_settings) {
    return absl::make_unique<AuthenticationManager>(std::move(user_settings));
}

} // namespace core
} // namespace game_launcher
