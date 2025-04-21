// core/src/user_settings.cpp
#include "game_launcher/core/user_settings.h"

#include <iostream> // For temporary error logging
#include <string>
#include <unordered_map>
#include <variant> // To store different types in the map

namespace game_launcher {
namespace core {

// Basic in-memory implementation of IUserSettings.
// TODO: Replace with a file-based implementation (e.g., JSON).
class InMemoryUserSettings : public IUserSettings {
 public:
  InMemoryUserSettings() = default;
  ~InMemoryUserSettings() override = default;

  bool LoadSettings() override {
    // For in-memory, loading is trivial (or could load defaults).
    std::cout << "InMemoryUserSettings: LoadSettings() called (no-op)." << std::endl;
    // settings_["default_install_path"] = "/path/to/games"; // Example default
    return true; // Always succeeds for now
  }

  bool SaveSettings() override {
    // For in-memory, saving is also a no-op in this basic version.
    std::cout << "InMemoryUserSettings: SaveSettings() called (no-op)." << std::endl;
    // In a file-based version, this would write the map to disk.
    return true; // Always succeeds for now
  }

  std::optional<std::string> GetString(std::string_view key) const override {
    auto it = settings_.find(std::string(key));
    if (it != settings_.end() && std::holds_alternative<std::string>(it->second)) {
      return std::get<std::string>(it->second);
    }
    return std::nullopt;
  }

  void SetString(std::string_view key, std::string_view value) override {
    settings_[std::string(key)] = std::string(value);
  }

  std::optional<int> GetInt(std::string_view key) const override {
    auto it = settings_.find(std::string(key));
    if (it != settings_.end() && std::holds_alternative<int>(it->second)) {
      return std::get<int>(it->second);
    }
    return std::nullopt;
  }

  void SetInt(std::string_view key, int value) override {
    settings_[std::string(key)] = value;
  }

  std::optional<bool> GetBool(std::string_view key) const override {
    auto it = settings_.find(std::string(key));
    if (it != settings_.end() && std::holds_alternative<bool>(it->second)) {
      return std::get<bool>(it->second);
    }
    return std::nullopt;
  }

  void SetBool(std::string_view key, bool value) override {
    settings_[std::string(key)] = value;
  }

 private:
  // Using std::variant to store different setting types in the map.
  using SettingValue = std::variant<std::string, int, bool>;
  std::unordered_map<std::string, SettingValue> settings_;
};

// TODO: Add a factory function or similar mechanism to create instances
// of the concrete UserSettings implementation when needed, potentially
// choosing between different implementations based on configuration or platform.
// For now, this definition is just within the .cpp file.

} // namespace core
} // namespace game_launcher

