// core/include/game_launcher/core/mock_auth_manager.h
#ifndef GAME_LAUNCHER_CORE_MOCK_AUTH_MANAGER_H_
#define GAME_LAUNCHER_CORE_MOCK_AUTH_MANAGER_H_

#include "game_launcher/core/authentication_manager.h"
#include "game_launcher/core/auth_status.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/memory/memory.h" // For std::unique_ptr
#include <string>
#include <memory> // For std::shared_ptr

namespace game_launcher {
namespace core {

// A minimal mock implementation of IAuthenticationManager for testing.
class MockAuthManager : public IAuthenticationManager {
public:
    MockAuthManager() = default; 
    ~MockAuthManager() override = default;

    // Mock implementations
    absl::Status Login(const std::string& username, const std::string& password) override;
    absl::Status Logout() override;
    AuthStatus GetAuthStatus() const override;
    absl::StatusOr<UserProfile> GetCurrentUserProfile() const override;
};

// Factory function (optional, but can be useful for consistency)
std::unique_ptr<IAuthenticationManager> CreateMockAuthManager();

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_MOCK_AUTH_MANAGER_H_
