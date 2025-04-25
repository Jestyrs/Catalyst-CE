// core/src/mock_auth_manager.cpp
#include "game_launcher/core/mock_auth_manager.h"
#include "game_launcher/core/user_profile.h"
#include "game_launcher/core/auth_status.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include <memory> // For std::make_shared and std::make_unique

namespace game_launcher {
namespace core {

absl::Status MockAuthManager::Login(const std::string& username, const std::string& password) {
    // Do nothing, always return OK for mock
    return absl::OkStatus();
}

absl::Status MockAuthManager::Logout() {
    // Do nothing, always return OK for mock
    return absl::OkStatus();
}

AuthStatus MockAuthManager::GetAuthStatus() const {
    // Always return Unauthenticated for mock
    return AuthStatus::kLoggedOut; // Corrected enum member
}

absl::StatusOr<UserProfile> MockAuthManager::GetCurrentUserProfile() const {
    // Always return NotFound for mock
    return absl::NotFoundError("User not logged in (mock)");
}

// Factory function implementation
std::unique_ptr<IAuthenticationManager> CreateMockAuthManager() {
    return std::make_unique<MockAuthManager>();
}

} // namespace core
} // namespace game_launcher
