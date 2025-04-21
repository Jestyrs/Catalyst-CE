#ifndef GAME_LAUNCHER_CORE_USER_SETTINGS_H_
#define GAME_LAUNCHER_CORE_USER_SETTINGS_H_

#include <optional>
#include <string>
#include <string_view>
#include <memory> // For factory return type

// Forward declaration for potential status objects later
// namespace absl { class Status; } 

namespace game_launcher {
namespace core {

// Interface for managing user settings.
// Implementations will handle loading from/saving to persistent storage.
class IUserSettings {
 public:
  virtual ~IUserSettings() = default;

  // Loads settings from the persistent source.
  // Returns status indicating success or failure.
  virtual bool LoadSettings() = 0; // TODO: Replace bool with absl::Status later

  // Saves the current settings to the persistent source.
  // Returns status indicating success or failure.
  virtual bool SaveSettings() = 0; // TODO: Replace bool with absl::Status later

  // Retrieves a string setting. Returns std::nullopt if the key is not found.
  virtual std::optional<std::string> GetString(std::string_view key) const = 0;

  // Sets a string setting.
  virtual void SetString(std::string_view key, std::string_view value) = 0;

  // Retrieves an integer setting. Returns std::nullopt if the key is not found or not an int.
  virtual std::optional<int> GetInt(std::string_view key) const = 0;

  // Sets an integer setting.
  virtual void SetInt(std::string_view key, int value) = 0;

  // Retrieves a boolean setting. Returns std::nullopt if the key is not found or not a bool.
  virtual std::optional<bool> GetBool(std::string_view key) const = 0;

  // Sets a boolean setting.
  virtual void SetBool(std::string_view key, bool value) = 0;

  // TODO: Add methods for other data types if needed (e.g., float, lists).
};

// Factory function to create an in-memory implementation of IUserSettings.
std::unique_ptr<IUserSettings> CreateInMemoryUserSettings();

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_USER_SETTINGS_H_
