// core/src/authentication_manager.cpp
#include "game_launcher/core/authentication_manager.h"
#include "game_launcher/core/user_settings.h" // Include for dependency
#include "absl/status/status.h"
#include "absl/strings/str_cat.h" // For error messages
#include "base/cef_logging.h"

#include <memory> // For std::make_unique
#include <string>
#include <optional>
// #include <iostream> // For debug logging

namespace game_launcher {
namespace core {

// Concrete implementation of IAuthenticationManager
class BasicAuthenticationManager : public IAuthenticationManager {
public:
    // Constructor taking dependencies (e.g., UserSettings)
    explicit BasicAuthenticationManager(std::shared_ptr<IUserSettings> user_settings)
        : user_settings_(user_settings), current_status_(AuthStatus::kLoggedOut) {
        // Try to load session from user_settings on startup
        LoadSession();
    }

    ~BasicAuthenticationManager() override = default;

    // --- IAuthenticationManager Implementation ---

    absl::Status Login(const std::string& username, const std::string& password) override {
        // --- Placeholder Implementation ---
        // In a real scenario, this would involve:
        // 1. Using an HTTP client library (e.g., CPR, cpprestsdk)
        // 2. Making a POST request to the backend /login endpoint with username/password.
        // 3. Handling the response (success with tokens/profile, or error).
        // 4. Securely storing the received session/refresh tokens (e.g., in UserSettings).

        if (username.empty() || password.empty()) {
            return absl::InvalidArgumentError("Username and password cannot be empty.");
        }

        // Simulate a successful login for specific dummy credentials
        if (username == "testuser" && password == "password") {
            current_status_ = AuthStatus::kLoggedIn;
            current_profile_ = UserProfile{
                .user_id = "user123",
                .username = username,
                .email = "testuser@example.com"
            };
            // Simulate saving session token
            if (user_settings_) {
                 absl::Status save_status = user_settings_->SetString("auth.session_token", "dummy_token_12345");
                 if (!save_status.ok()) {
                     // Log error: Failed to save session token. Depending on policy,
                     // might return save_status here or just log and continue.
                     // std::cerr << "Error saving session token: " << save_status << std::endl;
                 }
                 save_status = user_settings_->SetString("auth.user_id", current_profile_->user_id);
                 if (!save_status.ok()) { 
                     // Log error saving user_id
                     // std::cerr << "Error saving user_id: " << save_status << std::endl;
                 }
                 save_status = user_settings_->SetString("auth.username", current_profile_->username);
                  if (!save_status.ok()) { 
                      // Log error saving username
                      // std::cerr << "Error saving username: " << save_status << std::endl;
                  }
                 save_status = user_settings_->SetString("auth.email", current_profile_->email);
                  if (!save_status.ok()) { 
                      // Log error saving email
                      // std::cerr << "Error saving email: " << save_status << std::endl;
                  }
                 // UserSettings should have SaveSettings() called elsewhere if needed
            }
            return absl::OkStatus();
        } else {
            current_status_ = AuthStatus::kLoggedOut;
            current_profile_.reset();
            return absl::UnauthenticatedError("Invalid username or password.");
        }
    }

    absl::Status Logout() override {
        current_status_ = AuthStatus::kLoggedOut;
        current_profile_.reset();
         if (user_settings_) {
             absl::Status clear_status;
             // Clear stored session info by setting empty strings
             clear_status = user_settings_->SetString("auth.session_token", "");
             if (!clear_status.ok()) { 
                 // Log error clearing token
                 // std::cerr << "Error clearing token: " << clear_status << std::endl;
             }
             clear_status = user_settings_->SetString("auth.user_id", "");
             if (!clear_status.ok()) { 
                 // Log error clearing user_id
                 // std::cerr << "Error clearing user_id: " << clear_status << std::endl;
             }
             clear_status = user_settings_->SetString("auth.username", "");
              if (!clear_status.ok()) { 
                  // Log error clearing username
                  // std::cerr << "Error clearing username: " << clear_status << std::endl;
              }
             clear_status = user_settings_->SetString("auth.email", "");
              if (!clear_status.ok()) { 
                  // Log error clearing email
                  // std::cerr << "Error clearing email: " << clear_status << std::endl;
              }
             // UserSettings should have SaveSettings() called elsewhere if needed
         }
        return absl::OkStatus();
    }

    AuthStatus GetAuthStatus() const override {
        // TODO: Add logic to check token expiry if simulating that
        return current_status_;
    }

    absl::StatusOr<UserProfile> GetCurrentUserProfile() const override {
        if (current_status_ == AuthStatus::kLoggedIn && current_profile_) {
            return *current_profile_;
        } else {
            return absl::NotFoundError("No user is currently logged in.");
        }
    }

private:
    void LoadSession() {
         if (!user_settings_) return;

         auto token_status = user_settings_->GetString("auth.session_token");

         if (token_status.ok()) { // Token exists in settings
             if (token_status.value() == "dummy_token_12345") { // Token is valid
                 // Try loading profile data
                 auto user_id_status = user_settings_->GetString("auth.user_id");
                 auto username_status = user_settings_->GetString("auth.username");
                 auto email_status = user_settings_->GetString("auth.email");

                 if (user_id_status.ok() && username_status.ok() && email_status.ok()) {
                     // All data present, successfully load profile
                     current_status_ = AuthStatus::kLoggedIn;
                     current_profile_ = UserProfile{
                         .user_id = user_id_status.value(),
                         .username = username_status.value(),
                         .email = email_status.value()
                     };
                     // std::cout << "Loaded session for user: " << current_profile_->username << std::endl;
                     return; // Exit successfully
                 }
                 // If we reach here: Token was valid, but data was incomplete.
                 // We *don't* clear the settings in this case, just fall through
                 // to set the logged-out state below.

             } else if (!token_status.value().empty()) { // Token existed but was *invalid* (non-empty, non-matching)
                 LOG(INFO) << "Invalid session token found during initialization. Clearing stale auth data.";
                 absl::Status status;

                 status = user_settings_->SetString("auth.session_token", "");
                 if (!status.ok()) {
                     LOG(WARNING) << "Failed to clear session token setting: " << status;
                 }
                 status = user_settings_->SetString("auth.user_id", "");
                 if (!status.ok()) {
                     LOG(WARNING) << "Failed to clear user ID setting: " << status;
                 }
                 status = user_settings_->SetString("auth.username", "");
                 if (!status.ok()) {
                     LOG(WARNING) << "Failed to clear username setting: " << status;
                 }
                 status = user_settings_->SetString("auth.email", "");
                 if (!status.ok()) {
                     LOG(WARNING) << "Failed to clear email setting: " << status;
                 }
                 // Fall through to set logged-out state below.
             }
             // Case: Token existed but was empty -> treated like no token found (fallthrough)
         }

         // If token retrieval failed (not ok()), or token was empty, 
         // or token valid but data incomplete, or token invalid and cleared:
         // Ensure logged-out state.
         current_status_ = AuthStatus::kLoggedOut;
         current_profile_.reset();
    }


    std::shared_ptr<IUserSettings> user_settings_; // Dependency
    AuthStatus current_status_;
    std::optional<UserProfile> current_profile_;
};


// Factory function implementation
std::unique_ptr<IAuthenticationManager> CreateAuthenticationManager(std::shared_ptr<IUserSettings> user_settings) {
    if (!user_settings) {
        // Or handle this error more gracefully, maybe return a null/dummy manager?
        // For now, let's assume user_settings is required.
         return nullptr; // Indicate failure if dependency is missing
    }
    return std::make_unique<BasicAuthenticationManager>(user_settings);
}

} // namespace core
} // namespace game_launcher
