#ifndef GAME_LAUNCHER_CORE_USER_SETTINGS_H_
#define GAME_LAUNCHER_CORE_USER_SETTINGS_H_

#include <memory> // For factory return type
#include <string>
#include <vector>
#include <optional> // Include for std::optional

// Include Abseil status types
#include "absl/status/status.h"
#include "absl/status/statusor.h"

// Include AppSettings struct
#include "game_launcher/core/app_settings.h"

namespace game_launcher {
namespace core {

// Interface for managing user settings.
// Implementations will handle loading from/saving to persistent storage.
// Methods return absl::Status or absl::StatusOr<T> to indicate success/failure.
class IUserSettings {
 public:
  virtual ~IUserSettings() = default;

  // Loads settings from the persistent source.
  // Returns ok status on success, or an error status on failure.
  virtual absl::Status LoadSettings() = 0;

  // Saves the current settings to the persistent source.
  // Returns ok status on success, or an error status on failure.
  virtual absl::Status SaveSettings() = 0;

  // Retrieves the entire application settings structure.
  // If settings don't exist, returns a default-constructed AppSettings.
  virtual AppSettings GetAppSettings() const = 0;

  // Saves the entire application settings structure.
  // Returns ok status on success, error otherwise.
  virtual absl::Status SetAppSettings(const AppSettings& settings) = 0;

}; // class IUserSettings

// Factory function to create an in-memory implementation of IUserSettings.
std::unique_ptr<IUserSettings> CreateInMemoryUserSettings();

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_USER_SETTINGS_H_
