// core/include/game_launcher/core/authentication_manager.h
#ifndef GAME_LAUNCHER_CORE_AUTHENTICATION_MANAGER_H_
#define GAME_LAUNCHER_CORE_AUTHENTICATION_MANAGER_H_

#include "game_launcher/core/user_settings.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include <string>
#include <optional> // Include for potential future use
#include <memory>   // For std::unique_ptr in factory function

// Include the shared AuthStatus definition
#include "game_launcher/core/auth_status.h"
// Include the shared UserProfile definition
#include "game_launcher/core/user_profile.h"

namespace game_launcher {
namespace core {

class IAuthenticationManager {
public:
    virtual ~IAuthenticationManager() = default;

    // Attempt to log in using credentials via the backend API.
    // Returns OK status on success, error status otherwise.
    // On success, session tokens are stored internally.
    virtual absl::Status Login(const std::string& username, const std::string& password) = 0;

    // Log out the current user, clearing local session tokens.
    virtual absl::Status Logout() = 0;

    // Get the current authentication status.
    virtual AuthStatus GetAuthStatus() const = 0;

    // Get the profile of the currently logged-in user.
    // Returns profile data if logged in, or NotFoundError/UnauthenticatedError otherwise.
    virtual absl::StatusOr<UserProfile> GetCurrentUserProfile() const = 0;

    // TODO: Consider adding methods for:
    // - Register (might just open a web URL)
    // - RefreshToken (might happen automatically or need explicit call)
    // - GetAccessToken (to pass to other services/game client)
};

// Factory function to create an instance of the authentication manager
// Takes required dependencies (e.g., UserSettings)
std::unique_ptr<IAuthenticationManager> CreateAuthenticationManager(std::shared_ptr<IUserSettings> user_settings);

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_AUTHENTICATION_MANAGER_H_
