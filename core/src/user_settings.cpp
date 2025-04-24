// core/src/user_settings.cpp
#include "game_launcher/core/user_settings.h"
#include "game_launcher/core/json_user_settings.h"
#include "game_launcher/core/app_settings.h" // Include AppSettings definition

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <absl/status/status.h> // Include for absl::Status
#include <absl/status/statusor.h> // Include for absl::StatusOr
#include <absl/strings/str_cat.h> // Include for StrCat

namespace game_launcher::core {

// Forward declaration for the factory function
std::unique_ptr<IUserSettings> CreateInMemoryUserSettings();

/**
 * @brief Basic in-memory implementation of IUserSettings.
 */
class InMemoryUserSettings : public IUserSettings {
 public:
  InMemoryUserSettings() : current_settings_() {}
  ~InMemoryUserSettings() override = default;

  absl::Status LoadSettings() override {
    // In-memory implementation doesn't load/save from persistent storage
    return absl::OkStatus();
  }

  absl::Status SaveSettings() override {
    // In-memory implementation doesn't load/save from persistent storage
    return absl::OkStatus();
  }

  // Retrieves the entire application settings structure.
  AppSettings GetAppSettings() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_settings_;
  }

  // Saves the entire application settings structure.
  absl::Status SetAppSettings(const AppSettings& settings) override {
    std::lock_guard<std::mutex> lock(mutex_);
    current_settings_ = settings;
    // Note: No concept of 'dirty' for in-memory settings
    return absl::OkStatus();
  }

 private:
  AppSettings current_settings_;
  mutable std::mutex mutex_; // Protects current_settings_
};

// Factory function implementation
std::unique_ptr<IUserSettings> CreateInMemoryUserSettings() {
  return std::make_unique<InMemoryUserSettings>();
}

} // namespace game_launcher::core
