#ifndef GAME_LAUNCHER_CORE_CORE_IPC_SERVICE_H_
#define GAME_LAUNCHER_CORE_CORE_IPC_SERVICE_H_

#include "game_launcher/core/ipc_service.h"
#include "game_launcher/core/game_status_listener.h"
#include "game_launcher/core/authentication_manager.h" // Include Auth Manager interface
#include "game_launcher/core/user_settings.h"       // Include User Settings interface
#include "game_launcher/core/game_management_service.h" // Corrected include
#include "game_launcher/core/background_task_manager.h" // Add include
#include "absl/synchronization/mutex.h"

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include <map> // For managing game states
#include <unordered_map> // For task tracking
#include <thread> // For std::jthread
#include <mutex>  // For std::mutex, std::unique_lock, std::lock_guard
#include <condition_variable> // For std::condition_variable
#include <atomic> // For std::atomic<bool>
#include <vector> // Needed for iterating task IDs

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace game_launcher {
namespace core {

// Concrete implementation of the IIPCService interface
class CoreIPCService : public IIPCService {
public:
    /**
     * @brief Constructor.
     *
     * @param game_manager The game manager instance.
     * @param auth_manager The authentication manager instance.
     * @param user_settings The user settings instance.
     * @param background_task_manager The background task manager instance.
     */
    CoreIPCService(std::shared_ptr<IGameManagementService> game_manager,
                   std::unique_ptr<IAuthenticationManager> auth_manager,
                   std::shared_ptr<IUserSettings> user_settings,
                   IBackgroundTaskManager* background_task_manager);

    ~CoreIPCService();

    // Factory method to create an instance
    static std::unique_ptr<CoreIPCService> CreateCoreIPCService(
        std::shared_ptr<IGameManagementService> game_manager,
        std::unique_ptr<IAuthenticationManager> auth_manager,
        std::shared_ptr<IUserSettings> user_settings,
        IBackgroundTaskManager* background_task_manager);

    // IIPCService interface implementation
    absl::StatusOr<std::string> GetVersion();
    void AddStatusListener(IGameStatusListener* listener);
    void RemoveStatusListener(IGameStatusListener* listener);
    absl::StatusOr<std::vector<GameStatusUpdate>> GetInitialGameStatuses();
    absl::Status RequestInstall(std::string_view game_id);
    absl::Status RequestLaunch(std::string_view game_id);
    absl::Status RequestUpdate(std::string_view game_id);
    absl::Status RequestCancel(std::string_view game_id);
    absl::Status RequestExit();

    // Authentication methods
    absl::Status Login(std::string_view username, std::string_view password) override;
    absl::Status Logout() override;
    AuthStatus GetAuthStatus() const override;
    absl::StatusOr<UserProfile> GetCurrentUserProfile() const override;

    // Settings methods
    AppSettings GetAppSettings();
    absl::Status SetAppSettings(const AppSettings& settings);

    // Background Tasks
    absl::StatusOr<TaskStatus> GetTaskStatus(TaskId task_id);
    absl::Status CancelTask(TaskId task_id);

private:
    // Helper to notify all listeners
    void NotifyListeners(const GameStatusUpdate& update);
    // Helper to notify a single listener (used when adding)
    void NotifyListener(IGameStatusListener* listener, const std::vector<GameStatusUpdate>& updates);

    // Simulate game state management
    void InitializeGameStates();

    absl::Mutex listeners_mutex_;
    std::vector<IGameStatusListener*> listeners_ ABSL_GUARDED_BY(listeners_mutex_);

    // Internal state to manage game statuses (replace with real logic later)
    absl::Mutex games_mutex_;
    std::map<std::string, GameStatusUpdate> current_game_statuses_ ABSL_GUARDED_BY(games_mutex_);

    // Core service dependencies
    std::shared_ptr<IGameManagementService> game_manager_;
    std::unique_ptr<IAuthenticationManager> auth_manager_;
    std::shared_ptr<IUserSettings> user_settings_;
    IBackgroundTaskManager* background_task_manager_ = nullptr; // Non-owning pointer

    // Task tracking
    absl::Mutex task_tracking_mutex_; // Protects the two maps below
    absl::flat_hash_map<std::string, TaskId> active_game_tasks_ ABSL_GUARDED_BY(task_tracking_mutex_);
    absl::flat_hash_map<TaskId, TaskInfo> task_details_ ABSL_GUARDED_BY(task_tracking_mutex_); // Renamed and type corrected

    // Enum to distinguish task types
    enum class TaskOperationType {
        INSTALL,
        UPDATE
    };

    // Struct to hold details about an active task
    struct TaskDetails {
        std::string game_id;
        TaskOperationType operation_type;
    };

    // --- Background Task Monitoring --- 
    void MonitorTasksLoop();
    GameStatusUpdate TranslateTaskInfoToGameStatus(TaskId task_id, const TaskDetails& task_details, const TaskInfo& task_info);

    std::jthread monitor_thread_ ABSL_GUARDED_BY(task_tracking_mutex_);
    std::mutex monitor_mutex_; // Mutex specifically for monitor_cv_ synchronization
    std::condition_variable monitor_cv_;
    std::atomic<bool> stop_monitoring_;

    // Called periodically by the message loop
    bool PerformIdleTasks(); // Renamed from Idle
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_CORE_IPC_SERVICE_H_
