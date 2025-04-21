// core/include/game_launcher/core/json_user_settings.h
#ifndef GAME_LAUNCHER_CORE_JSON_USER_SETTINGS_H_
#define GAME_LAUNCHER_CORE_JSON_USER_SETTINGS_H_

#include "game_launcher/core/user_settings.h"
#include "third_party/nlohmann/json.hpp" // Include the JSON library header

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
  bool LoadSettings() override;

  // Saves the current settings to the JSON file. Returns true on success.
  // Only writes if changes have been made since the last load/save.
  bool SaveSettings() override;

  // Retrieves settings. Returns std::nullopt if key not found or type mismatch.
  std::optional<std::string> GetString(std::string_view key) const override;
  std::optional<int> GetInt(std::string_view key) const override;
  std::optional<bool> GetBool(std::string_view key) const override;

  // Sets settings. Marks settings as dirty, requiring a save.
  void SetString(std::string_view key, std::string_view value) override;
  void SetInt(std::string_view key, int value) override;
  void SetBool(std::string_view key, bool value) override;

  // Checks if a key exists.
  bool HasKey(std::string_view key) const override;

  // Removes a specific key.
  bool RemoveKey(std::string_view key) override;

  // Clears all settings in memory (does not delete the file until SaveSettings is called).
  void Clear() override;

 private:
  // Helper to get a setting node with type checking.
  // Returns nullptr if key not found or type mismatch.
  const nlohmann::json* GetSettingNode(std::string_view key, nlohmann::json::value_t expected_type) const;

  const std::filesystem::path settings_file_path_;
  nlohmann::json settings_json_;
  mutable std::mutex mutex_; // Protects settings_json_ and dirty_flag_
  bool dirty_flag_ = false;  // Tracks if changes need to be saved
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_JSON_USER_SETTINGS_H_
