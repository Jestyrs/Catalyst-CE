#ifndef GAME_LAUNCHER_CORE_BASIC_GAME_MANAGEMENT_SERVICE_H_
#define GAME_LAUNCHER_CORE_BASIC_GAME_MANAGEMENT_SERVICE_H_

#include "game_launcher/core/game_management_service.h"
#include "game_launcher/core/user_settings.h"
#include "game_launcher/core/game_status.h"
#include "game_launcher/core/background_task_manager.h"
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h" // Include for absl::Mutex
#include "absl/container/flat_hash_map.h" // Include for absl::flat_hash_map
#include "nlohmann/json_fwd.hpp" // Forward declaration

namespace game_launcher {
namespace core {

// Basic placeholder implementation of IGameManagementService.
class BasicGameManagementService : public IGameManagementService {
public:
    // Constructor takes the path to the games JSON file, user settings, and background task manager.
    explicit BasicGameManagementService(const std::filesystem::path& games_json_path,
                                          std::shared_ptr<IUserSettings> user_settings,
                                          IBackgroundTaskManager* background_task_manager);

    ~BasicGameManagementService() override = default;

    // IGameManagementService overrides
    absl::StatusOr<std::vector<GameInfo>> GetInstalledGames() const override;
    absl::StatusOr<GameInfo> GetGameDetails(std::string_view game_id) const override;
    absl::Status InstallGame(std::string_view game_id, std::string_view manifest_url) override;
    absl::Status UpdateGame(std::string_view game_id) override;
    absl::Status LaunchGame(std::string_view game_id) override;
    absl::Status UninstallGame(std::string_view game_id) override; 
    absl::Status CancelOperation(std::string_view game_id) override;

private:
    // Loads game data from the specified JSON file.
    absl::Status LoadGamesFromFile(const std::filesystem::path& file_path);

    // Helper to get the designated install path for a game
    absl::StatusOr<std::filesystem::path> GetGameInstallPath(std::string_view game_id_sv);

    absl::Status VerifyGame(std::string_view game_id);
    absl::Status VerifyGameFiles(std::string_view game_id);
    absl::Status VerifyFileHash(const std::filesystem::path& file_path, const std::string& expected_hash);
    absl::Status SaveLocalManifest(std::string_view game_id_sv, const std::filesystem::path& install_path, const nlohmann::json& manifest_data);
    absl::StatusOr<nlohmann::json> LoadLocalManifest(std::string_view game_id_sv, const std::filesystem::path& install_path) const;

    // Helper function to find the intended installation path for a given game ID.
    absl::StatusOr<std::filesystem::path> FindInstallPath(std::string_view game_id);

    // Helper function to get the base directory for game installations.
    std::filesystem::path GetBaseInstallDirectory();

    // Helper function to ensure a directory exists, creating it if necessary.
    absl::Status EnsureDirectoryExists(const std::filesystem::path& dir_path);

    // Internal handlers (e.g., called by background task callbacks)
    void HandleDownloadProgress(std::string_view game_id_sv, int64_t total_bytes, int64_t transferred_bytes, double bytes_per_second);
    absl::Status HandleDownloadCompletion(std::string_view game_id_sv);

    std::shared_ptr<IUserSettings> user_settings_;
    IBackgroundTaskManager* background_task_manager_; // Non-owning pointer

    // Map storing game data, keyed by game ID.
    std::map<std::string, GameInfo> loaded_games_;

    // Mutex and map to track active operations (install/update) per game
    absl::Mutex active_operations_mutex_;
    absl::flat_hash_map<std::string, TaskId> active_operations_ ABSL_GUARDED_BY(active_operations_mutex_);

    // Private helper methods
    absl::Status DownloadFile(const std::string& url, const std::filesystem::path& destination_path);

    // Listener management removed - handled by CoreIPCService

};

// Factory function to create instances of BasicGameManagementService.
std::shared_ptr<IGameManagementService> CreateBasicGameManager(
    const std::filesystem::path& games_json_path,
    std::shared_ptr<IUserSettings> user_settings,
    IBackgroundTaskManager* background_task_manager);

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_BASIC_GAME_MANAGEMENT_SERVICE_H_
