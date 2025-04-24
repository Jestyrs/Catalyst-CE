#ifndef GAME_LAUNCHER_CORE_GAME_MANAGEMENT_SERVICE_H_
#define GAME_LAUNCHER_CORE_GAME_MANAGEMENT_SERVICE_H_

#include <string>
#include <string_view>
#include <vector>
#include <optional> // Could be used for optional fields later

// Include Abseil status types
#include "absl/status/status.h"
#include "absl/status/statusor.h"

// Include background task manager for ProgressReporter
#include "game_launcher/core/background_task_manager.h"

namespace game_launcher {
namespace core {

// Structure to hold information about a game.
struct GameInfo {
    std::string id;             // Unique identifier for the game (e.g., internal name or UUID)
    std::string name;           // Display name of the game
    std::string install_path;   // Root installation directory
    std::string executable_path;// Path to the main game executable relative to install_path
    std::string version;        // Installed version string (can be empty)
    // Add other fields as needed: icon path, required disk space, etc.
};

// Interface for managing game installations and launching.
class IGameManagementService {
public:
    virtual ~IGameManagementService() = default;

    // Retrieves a list of all known installed games.
    // Returns StatusOr containing the list on success, or an error status.
    virtual absl::StatusOr<std::vector<GameInfo>> GetInstalledGames() const = 0;

    // Retrieves detailed information for a specific game by its ID.
    // Returns StatusOr containing the GameInfo if found, or an error status (e.g., kNotFound).
    virtual absl::StatusOr<GameInfo> GetGameDetails(std::string_view game_id) const = 0;

    // Launches the specified game by its ID.
    // Returns ok status on successful launch initiation, or an error status.
    // Note: This might involve creating a new process and returning immediately,
    // or it could potentially wait for the process (TBD).
    virtual absl::Status LaunchGame(std::string_view game_id) = 0;

    // Installs a game identified by game_id using the provided manifest URL.
    // Progress is reported via the provided reporter.
    virtual absl::Status InstallGame(std::string_view game_id, 
                                      std::string_view manifest_url) = 0;

    // Updates an existing game identified by game_id.
    // This typically involves fetching a new manifest, comparing, downloading patches/files.
    virtual absl::Status UpdateGame(std::string_view game_id) = 0;

    // Uninstalls a game identified by game_id.
    virtual absl::Status UninstallGame(std::string_view game_id) = 0;

    // Cancels an ongoing operation (install/update) for the specified game.
    virtual absl::Status CancelOperation(std::string_view game_id) = 0;

    // Future methods could include:
    // virtual absl::Status VerifyGameFiles(std::string_view game_id) = 0;

}; // class IGameManagementService

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_GAME_MANAGEMENT_SERVICE_H_
