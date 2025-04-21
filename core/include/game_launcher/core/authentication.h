#ifndef GAME_LAUNCHER_CORE_AUTHENTICATION_H_
#define GAME_LAUNCHER_CORE_AUTHENTICATION_H_

#include <optional>
#include <string>
#include <string_view>

// Forward declarations
namespace game_launcher { namespace core { class IUserSettings; } } 
// namespace absl { class Status; }

namespace game_launcher {
namespace core {

// Interface for handling user authentication and session management.
class IAuthentication {
 public:
  virtual ~IAuthentication() = default;

  // Attempts to log in the user with the provided credentials.
  // Returns status indicating success or failure.
  // On success, session information should be stored securely.
  virtual bool Login(std::string_view username, std::string_view password) = 0; // TODO: Replace bool with absl::Status

  // Logs out the currently authenticated user.
  // Clears session information.
  // Returns status indicating success or failure.
  virtual bool Logout() = 0; // TODO: Replace bool with absl::Status

  // Checks if a user is currently logged in.
  virtual bool IsLoggedIn() const = 0;

  // Gets the username of the currently logged-in user.
  // Returns std::nullopt if no user is logged in.
  virtual std::optional<std::string> GetUsername() const = 0;

  // Gets the current session token or API key.
  // Returns std::nullopt if no user is logged in or token is unavailable.
  // Note: Exposing raw tokens requires careful consideration of security implications.
  virtual std::optional<std::string> GetSessionToken() const = 0;

  // Allows setting dependencies like UserSettings.
  // Demonstrates dependency injection.
  virtual void SetUserSettings(IUserSettings* user_settings) = 0;

  // TODO: Add methods for token refresh, handling different auth flows (OAuth), etc.
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_AUTHENTICATION_H_
