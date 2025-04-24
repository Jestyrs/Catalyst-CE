// core/include/game_launcher/core/json_user_settings.h
#ifndef GAME_LAUNCHER_CORE_JSON_USER_SETTINGS_H_
#define GAME_LAUNCHER_CORE_JSON_USER_SETTINGS_H_

#include "game_launcher/core/user_settings.h"
#include "game_launcher/core/app_settings.h" // Include the AppSettings struct
#include "nlohmann/json.hpp" // Include the JSON library header (path set by CMake)
#include "absl/status/status.h" // Include absl::Status
#include "absl/status/statusor.h" // Include absl::StatusOr

#include <filesystem> // For std::filesystem::path
#include <mutex>      // For std::mutex
#include <string>

namespace game_launcher {
namespace core {

// Implementation of IUserSettings that saves/loads settings to a JSON file.
class JsonUserSettings : public IUserSettings {
 public:
  // Constructor takes the path to the settings file.
  explicit JsonUserSettings(std::filesystem::path settings_file_path);

  // Delete copy/move constructors and assignment operators
  JsonUserSettings(const JsonUserSettings&) = delete;
  JsonUserSettings& operator=(const JsonUserSettings&) = delete;
  JsonUserSettings(JsonUserSettings&&) = delete;
  JsonUserSettings& operator=(JsonUserSettings&&) = delete;

  ~JsonUserSettings() override;

  // --- IUserSettings Interface Implementation ---

  // Loads settings from the JSON file. Returns true on success, false on failure.
  // Creates the file with default empty JSON {} if it doesn't exist.
  absl::Status LoadSettings() override;

  // Saves the current settings to the JSON file. Returns true on success.
  // Only writes if changes have been made since the last load/save.
  absl::Status SaveSettings() override;

  // Retrieves the current application settings structure.
  AppSettings GetAppSettings() const override;

  // Updates the application settings structure.
  absl::Status SetAppSettings(const AppSettings& settings) override;

 private:
  // Helper to ensure settings file directory exists.
  absl::Status EnsureDirectoryExists();
  // Helper to read JSON from file.
  absl::StatusOr<nlohmann::json> ReadJsonFromFile();
  // Helper to write JSON to file.
  absl::Status WriteJsonToFile(const nlohmann::json& json_data);

  const std::filesystem::path settings_file_path_;
  AppSettings current_settings_; // Holds the settings currently in memory
  mutable std::mutex mutex_; // Mutex to protect access to settings data
  bool settings_dirty_ = false; // Flag to track if settings need saving
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_JSON_USER_SETTINGS_H_
