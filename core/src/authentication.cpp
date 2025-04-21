// core/src/authentication.cpp
#include "game_launcher/core/authentication.h"
#include "game_launcher/core/user_settings.h" // Needed for dependency

#include <iostream> // For temporary logging
#include <string>
#include <optional>

namespace game_launcher {
namespace core {

// Basic in-memory implementation of IAuthentication.
// TODO: Replace with a real implementation involving secure storage and server communication.
class InMemoryAuthentication : public IAuthentication {
 public:
  InMemoryAuthentication() : user_settings_(nullptr), is_logged_in_(false) {}
  ~InMemoryAuthentication() override = default;

  // Non-copyable and non-movable (or define explicitly if needed)
  InMemoryAuthentication(const InMemoryAuthentication&) = delete;
  InMemoryAuthentication& operator=(const InMemoryAuthentication&) = delete;
  InMemoryAuthentication(InMemoryAuthentication&&) = delete;
  InMemoryAuthentication& operator=(InMemoryAuthentication&&) = delete;


  bool Login(std::string_view username, std::string_view password) override {
    std::cout << "InMemoryAuthentication: Attempting login for user: "
              << username << std::endl;

    // --- VERY INSECURE - Placeholder logic ---
    // In a real implementation:
    // 1. Hash the provided password.
    // 2. Send username and hashed password to an authentication server securely (HTTPS).
    // 3. Server verifies credentials against its database.
    // 4. Server returns a session token (e.g., JWT) on success.
    // 5. Securely store the session token (using platform APIs - see Table 1 from plan).
    // 6. Store username and potentially other non-sensitive session info.

    if (username == "testuser" && password == "password") {
      std::cout << "InMemoryAuthentication: Login successful (placeholder)." << std::endl;
      is_logged_in_ = true;
      current_username_ = std::string(username);
      // Store a dummy token for demonstration
      session_token_ = "dummy-token-" + std::string(username);

      // Example of using the injected dependency
      if (user_settings_) {
          user_settings_->SetString("last_logged_in_user", username);
          user_settings_->SaveSettings(); // Or queue save
      }

      return true;
    } else {
      std::cout << "InMemoryAuthentication: Login failed (placeholder)." << std::endl;
      is_logged_in_ = false;
      current_username_.reset();
      session_token_.reset();
      return false;
    }
    // --- End Placeholder logic ---
  }

  bool Logout() override {
     std::cout << "InMemoryAuthentication: Logout called." << std::endl;
     // In a real implementation:
     // 1. Invalidate the session token on the server if possible.
     // 2. Securely delete the stored session token and related data.
     is_logged_in_ = false;
     current_username_.reset();
     session_token_.reset();
     return true; // Always succeeds for now
  }

  bool IsLoggedIn() const override {
    return is_logged_in_;
  }

  std::optional<std::string> GetUsername() const override {
    return current_username_;
  }

  std::optional<std::string> GetSessionToken() const override {
    // WARNING: Returning raw tokens might be a security risk depending on usage.
    // Consider returning opaque handles or derived info if possible.
    return session_token_;
  }

  void SetUserSettings(IUserSettings* user_settings) override {
      user_settings_ = user_settings;
  }

 private:
  IUserSettings* user_settings_; // Non-owning pointer to dependency
  bool is_logged_in_;
  std::optional<std::string> current_username_;
  std::optional<std::string> session_token_; // Placeholder for session token
};

// TODO: Add a factory function or similar mechanism to create instances
// of the concrete Authentication implementation when needed.

} // namespace core
} // namespace game_launcher