#ifndef GAME_LAUNCHER_CORE_APP_SETTINGS_H_
#define GAME_LAUNCHER_CORE_APP_SETTINGS_H_

#include <string>
#include <vector>
#include <optional>
#include <filesystem> // Include for std::filesystem::path if needed later
#include <nlohmann/json.hpp> // Include the JSON library header

namespace game_launcher {
namespace core {

/**
 * @brief Structure to hold application-wide settings.
 */
struct AppSettings {
    // Consolidate fields from previous definition
    std::string install_path = "";
    std::string language = "en";
    std::vector<std::string> game_ids; // Example: List of installed game IDs
    bool auto_update_launcher = true;
    std::optional<std::string> user_profile_json; // For storing serialized user profile

    // Add comparison operator for testing or comparison needs (optional but recommended)
    bool operator==(const AppSettings& other) const {
        return install_path == other.install_path &&
               language == other.language &&
               game_ids == other.game_ids &&
               auto_update_launcher == other.auto_update_launcher &&
               user_profile_json == other.user_profile_json; // std::optional has operator==
    }
};

// Declare JSON serialization functions (definitions are in app_settings.cpp)
void to_json(nlohmann::json& j, const AppSettings& s);
void from_json(const nlohmann::json& j, AppSettings& s);

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_APP_SETTINGS_H_
