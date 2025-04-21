// core/include/game_launcher/core/game_management.h
#ifndef GAME_LAUNCHER_CORE_GAME_MANAGEMENT_H_
#define GAME_LAUNCHER_CORE_GAME_MANAGEMENT_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations
namespace game_launcher { namespace core {
    class IUserSettings;
    class IBackgroundTaskManager;
    // Potentially define structs for GameInfo, Progress, etc.
}}
// namespace absl { class Status; }

namespace game_launcher {
namespace core {

// TODO: Define helper structs if needed
struct GameInfo {
    std::string id;         // Unique identifier for the game
    std::string name;
    std::string install_path;
    std::string current_version;
    // Add other relevant info: icon path, executable path, install status, etc.
};

// Interface for managing game lifecycle operations.
class IGameManagement {
 public:
  virtual ~IGameManagement() = default;

  // Discovers currently installed games.
  // Might scan directories defined in UserSettings.
  virtual std::vector<GameInfo> DiscoverInstalledGames() = 0;

  // Gets detailed information for a specific game.
  virtual std::optional<GameInfo> GetGameInfo(std::string_view game_id) = 0;

  // Initiates the installation process for a game.
  // Returns status/task ID for tracking progress (via BackgroundTaskManager).
  // Requires source URL/manifest, install location, etc.
  virtual bool InstallGame(std::string_view game_id, std::string_view install_path) = 0; // TODO: Refine signature, return Status/TaskID

  // Initiates the update/patching process for an installed game.
  // Returns status/task ID for tracking progress.
  virtual bool UpdateGame(std::string_view game_id) = 0; // TODO: Return Status/TaskID

  // Validates the integrity of game files and attempts repairs if necessary.
  // Returns status/task ID for tracking progress.
  virtual bool ValidateGame(std::string_view game_id) = 0; // TODO: Return Status/TaskID

  // Launches the specified game.
  // May involve anti-cheat initialization, command-line args, etc.
  // Returns status indicating success/failure of the launch attempt.
  virtual bool LaunchGame(std::string_view game_id) = 0; // TODO: Return Status

  // Allows setting dependencies.
  virtual void SetUserSettings(IUserSettings* user_settings) = 0;
  virtual void SetBackgroundTaskManager(IBackgroundTaskManager* task_manager) = 0;

  // TODO: Add methods for uninstalling games.
  // TODO: Add methods for querying download/patch progress directly (or rely on BackgroundTaskManager).
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_GAME_MANAGEMENT_H_
