#include "game_launcher/core/app_settings.h"

#include <nlohmann/json.hpp>

namespace game_launcher {
namespace core {

// Define how AppSettings is serialized/deserialized to/from JSON
// Moved definitions from app_settings.h

void to_json(nlohmann::json& j, const AppSettings& s) {
    j = nlohmann::json{
        {"install_path", s.install_path},
        {"language", s.language},
        {"game_ids", s.game_ids},
        {"auto_update_launcher", s.auto_update_launcher}
        // user_profile_json handled below
    };
    // Handle optional field: serialize as null if empty
    if (s.user_profile_json.has_value()) {
        j["user_profile_json"] = s.user_profile_json.value();
    } else {
        j["user_profile_json"] = nullptr; // Explicitly set to JSON null
    }
}

void from_json(const nlohmann::json& j, AppSettings& s) {
    j.at("install_path").get_to(s.install_path);
    j.at("language").get_to(s.language);
    j.at("game_ids").get_to(s.game_ids);
    j.at("auto_update_launcher").get_to(s.auto_update_launcher);

    // Handle optional field: check existence and non-null before getting
    if (j.contains("user_profile_json") && !j.at("user_profile_json").is_null()) {
        // Use emplace to construct the string directly if needed, or assign
        s.user_profile_json = j.at("user_profile_json").get<std::string>();
    } else {
        s.user_profile_json = std::nullopt; // Assign nullopt if key missing or null
    }
}

} // namespace core
} // namespace game_launcher
