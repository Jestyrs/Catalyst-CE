#ifndef GAME_LAUNCHER_CORE_USER_PROFILE_H_
#define GAME_LAUNCHER_CORE_USER_PROFILE_H_

#include <string>
#include "nlohmann/json.hpp" // Include JSON library

namespace game_launcher {
namespace core {

/**
 * @brief Represents basic user profile information retrieved from the backend.
 */
struct UserProfile {
    std::string user_id;
    std::string username;
    std::string email; // Optional, depending on API
    // Add other relevant profile fields as needed (e.g., display name)

    // Add comparison operator for testing or comparison needs (optional but recommended)
    bool operator==(const UserProfile& other) const = default;
};

// Declare JSON conversion functions (definitions in user_profile.cpp)
void to_json(nlohmann::json& j, const UserProfile& p);
void from_json(const nlohmann::json& j, UserProfile& p);

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_USER_PROFILE_H_
