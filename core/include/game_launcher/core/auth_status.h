#ifndef GAME_LAUNCHER_CORE_AUTH_STATUS_H_
#define GAME_LAUNCHER_CORE_AUTH_STATUS_H_

#include <string>

namespace game_launcher {
namespace core {

/**
 * @brief Represents the authentication status of the user.
 */
enum class AuthStatus {
    kUnknown,     // Initial state or status uncertain
    kLoggedOut,   // User is explicitly logged out
    kLoggingIn,   // Login process is in progress (optional)
    kLoggedIn,    // User is successfully logged in
    kLoggingOut,  // Logout process is in progress (optional)
    kError        // An error occurred during authentication
};

// Helper function to convert AuthStatus enum to string
inline std::string AuthStatusToString(AuthStatus status) {
    switch (status) {
        case AuthStatus::kUnknown:    return "Unknown";
        case AuthStatus::kLoggedOut:  return "LoggedOut";
        case AuthStatus::kLoggingIn:  return "LoggingIn";
        case AuthStatus::kLoggedIn:   return "LoggedIn";
        case AuthStatus::kLoggingOut: return "LoggingOut";
        case AuthStatus::kError:      return "Error";
        default:                     return "InvalidStatus";
    }
}

} // namespace core

} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_AUTH_STATUS_H_
