// core/src/user_profile.cpp

#include "game_launcher/core/user_profile.h"

namespace game_launcher {
namespace core {

// Define how to convert UserProfile to JSON
void to_json(nlohmann::json& j, const UserProfile& p) {
    j = nlohmann::json{{"user_id", p.user_id},
                           {"username", p.username},
                           {"email", p.email}};
}

// Define how to convert UserProfile from JSON
void from_json(const nlohmann::json& j, UserProfile& p) {
    j.at("user_id").get_to(p.user_id);
    j.at("username").get_to(p.username);
    // Handle potentially missing email gracefully
    if (j.contains("email")) {
        j.at("email").get_to(p.email);
    } else {
        p.email = ""; // Or handle as appropriate
    }
}

} // namespace core
} // namespace game_launcher
