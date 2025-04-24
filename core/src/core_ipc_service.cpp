#include "game_launcher/core/core_ipc_service.h"

#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "absl/strings/str_format.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "absl/time/time.h"
#include "absl/status/status.h"
#include "absl/log/log.h"
#include "game_launcher/core/game_status.h"
#include "game_launcher/core/game_management_service.h"
#include "game_launcher/core/background_task_manager.h"
// #include "game_launcher/core/auth_service.h" // File not found

namespace game_launcher {
namespace core {

// --- Constructor, Destructor, Factory --- 

CoreIPCService::CoreIPCService(
    std::shared_ptr<IGameManagementService> game_manager,
    std::unique_ptr<IAuthenticationManager> auth_manager,
    std::shared_ptr<IUserSettings> user_settings,
    IBackgroundTaskManager* background_task_manager)
    : game_manager_(std::move(game_manager)),
      auth_manager_(std::move(auth_manager)),
      user_settings_(std::move(user_settings)),
      background_task_manager_(background_task_manager),
      stop_monitoring_(false) // Ensure stop flag is initially false
      {
    LOG(INFO) << "CoreIPCService initializing...";
    if (!background_task_manager_) {
        LOG(FATAL) << "CoreIPCService requires a valid BackgroundTaskManager instance.";
        // Or throw an exception, depending on error handling strategy
    }
    InitializeGameStates();
    monitor_thread_ = std::jthread(&CoreIPCService::MonitorTasksLoop, this); 
    LOG(INFO) << "CoreIPCService initialization complete. Monitor thread started.";
}

CoreIPCService::~CoreIPCService() {
    LOG(INFO) << "CoreIPCService shutting down...";
    stop_monitoring_.store(true); // Signal the thread to stop
    monitor_cv_.notify_one();    // Wake up the thread if it's waiting
    // jthread automatically joins upon destruction, no explicit join needed unless forced.
    LOG(INFO) << "CoreIPCService monitor thread stopped.";
}

std::unique_ptr<CoreIPCService> CoreIPCService::CreateCoreIPCService(
    std::shared_ptr<IGameManagementService> game_manager,
    std::unique_ptr<IAuthenticationManager> auth_manager,
    std::shared_ptr<IUserSettings> user_settings,
    IBackgroundTaskManager* background_task_manager) {
    return std::make_unique<CoreIPCService>(std::move(game_manager), 
                                          std::move(auth_manager),
                                          std::move(user_settings),
                                          background_task_manager);
}

// --- Initialization --- 
void CoreIPCService::InitializeGameStates() {
    LOG(INFO) << "Initializing game states from GameManagementService...";
    absl::MutexLock lock(&games_mutex_); // Lock for duration of initialization
    current_game_statuses_.clear(); // Clear any previous dummy data

    absl::StatusOr<std::vector<GameInfo>> installed_games_status = game_manager_->GetInstalledGames();

    if (!installed_games_status.ok()) {
        LOG(ERROR) << "Failed to get installed games list: " << installed_games_status.status();
        // Decide how to handle this - maybe start with an empty list? Or propagate error?
        // For now, log the error and continue with an empty list.
        return;
    }

    const std::vector<GameInfo>& installed_games = *installed_games_status;
    LOG(INFO) << "Found " << installed_games.size() << " installed games.";

    for (const auto& game_info : installed_games) {
        GameStatusUpdate status;
        status.game_id = game_info.id;
        // Assume games returned by GetInstalledGames are ready/installed.
        // We might need a more nuanced state from GameInfo later (e.g., needs_update).
        status.current_state = GameState::ReadyToLaunch; // Use ReadyToLaunch for installed/up-to-date.
        status.progress_percent = 100; // Explicitly set 100% for ready state
        status.message = "Ready to Play";
        // Title/description are part of GameInfo, not GameStatusUpdate.
        // UI should fetch details separately if needed.

        current_game_statuses_[game_info.id] = status;
        LOG(INFO) << "Initialized status for game: " << game_info.id << " as ReadyToLaunch.";
    }

    LOG(INFO) << "Game state initialization complete.";
}

// --- Action Requests ---

absl::Status CoreIPCService::RequestInstall(std::string_view game_id_sv) {
    // ... existing code ...
    return absl::OkStatus();
}

absl::Status CoreIPCService::RequestLaunch(std::string_view game_id_sv) {
    std::string game_id_str(game_id_sv);
    LOG(INFO) << "Received request to launch game: " << game_id_str;

    // Directly call the game manager to attempt launch.
    absl::Status launch_status = game_manager_->LaunchGame(game_id_str);

    if (launch_status.ok()) {
        LOG(INFO) << "Game manager accepted launch request for game: " << game_id_str;
        // Update status locally and notify listeners immediately.
        // We assume LAUNCHING state here. Monitoring the actual game process
        // lifecycle (RUNNING, Exited) would be a separate, more complex feature.
        absl::MutexLock lock(&games_mutex_);
        auto it = current_game_statuses_.find(game_id_str);
        if (it != current_game_statuses_.end()) {
            it->second.current_state = GameState::Launching; 
            it->second.progress_percent = 0; // Progress not applicable for launching itself
            it->second.message = "Launching...";
            GameStatusUpdate update = it->second; // Copy before unlock
            NotifyListeners(update); // Notify listeners about the state change
        } else {
            // This case should ideally not happen if the UI only shows launchable games
            // based on GetInitialGameStatuses, but handle defensively.
            LOG(WARNING) << "RequestLaunch called for unknown or non-initialized game ID: " << game_id_str;
            // Optionally return an error or proceed without notification?
            // For now, we proceed, but the status might not be tracked.
        }
    } else {
        LOG(ERROR) << "Game manager failed to initiate launch for game " << game_id_str 
                      << ": " << launch_status;
        // Optionally, update status to FAILED_TO_LAUNCH and notify?
        // For now, we just return the error from the game manager.
    }

    return launch_status; 
}

absl::Status CoreIPCService::RequestUpdate(std::string_view game_id_sv) {
    // ... existing code ...
    LOG(INFO) << "RequestUpdate for game: " << game_id_sv << " (Not Implemented)";
    return absl::UnimplementedError("Update not implemented");
}

absl::Status CoreIPCService::RequestCancel(std::string_view game_id_sv) {
    std::string game_id(game_id_sv);
    LOG(INFO) << "Received request to cancel operation for game: " << game_id;

    // Find the task associated with the game_id
    TaskId task_id_to_cancel = kInvalidTaskId;
    {
        absl::MutexLock lock(&task_tracking_mutex_);
        auto it = active_game_tasks_.find(game_id);
        if (it != active_game_tasks_.end()) {
            task_id_to_cancel = it->second;
            // Remove from active tasks immediately to prevent new listeners seeing it
            // We might refine this later - perhaps leave it until confirmed cancelled?
            active_game_tasks_.erase(it);
            // Also remove from task_details_
            task_details_.erase(task_id_to_cancel);
        } else {
            LOG(WARNING) << "RequestCancel: No active operation found for game ID: " << game_id;
            return absl::NotFoundError(absl::StrCat("No active operation found for game: ", game_id));
        }
    }

    if (task_id_to_cancel != kInvalidTaskId) {
        LOG(INFO) << "Requesting cancellation for Task ID: " << task_id_to_cancel;
        background_task_manager_->RequestCancellation(task_id_to_cancel); // Returns void

        // Assume cancellation request was accepted. The task manager will handle it.
        // Update game state locally to Idle immediately.
        LOG(INFO) << "Cancellation requested for Task ID: " << task_id_to_cancel << ". Updating local state to Idle.";
        {
            absl::MutexLock lock(&games_mutex_);
            auto status_it = current_game_statuses_.find(game_id);
            if (status_it != current_game_statuses_.end()) {
                status_it->second.current_state = GameState::Idle; // Using Idle now
                status_it->second.progress_percent = 0;
                status_it->second.message = "Operation Canceled";
                NotifyListeners(status_it->second); // Notify listeners about the change
            }
            // else: Game status wasn't found? Should we log?
        } // games_mutex_ released

        // Return OK status because the *request* was successfully submitted.
        // The actual cancellation result will be observed via task monitoring.
        return absl::OkStatus();
    } else {
        // Should be caught by the check above, but handle defensively.
        return absl::InternalError("Failed to find Task ID for cancellation after initial check.");
    }
}

absl::StatusOr<std::string> CoreIPCService::GetVersion() {
    LOG(INFO) << "GetVersion called (Returning placeholder).";
    // TODO: Implement actual version retrieval (e.g., from build info)
    return std::string("0.1.0-alpha"); 
}

absl::StatusOr<std::vector<GameStatusUpdate>> CoreIPCService::GetInitialGameStatuses() {
    LOG(INFO) << "GetInitialGameStatuses called.";
    absl::MutexLock lock(&games_mutex_);
    std::vector<GameStatusUpdate> statuses;
    statuses.reserve(current_game_statuses_.size());
    for (const auto& pair : current_game_statuses_) {
        statuses.push_back(pair.second);
    }
    LOG(INFO) << "Returning " << statuses.size() << " initial game statuses.";
    return statuses;
}

absl::Status CoreIPCService::Login(std::string_view username, std::string_view password) {
    // LOG(INFO) << "Login called for user: " << username;
    // if (!auth_manager_) { // Assuming auth_manager_ is related to auth_service.h
    //     return absl::InternalError("Authentication manager is not initialized.");
    // }
    // return auth_manager_->Login(username, password);
    return absl::UnimplementedError("Login functionality requires auth_service.h which was not found.");
}

absl::Status CoreIPCService::Logout() {
    LOG(INFO) << "Logout called.";
    if (!auth_manager_) {
        return absl::InternalError("Authentication manager is not initialized.");
    }
    // Delegate to the authentication manager
    return auth_manager_->Logout();
}

AuthStatus CoreIPCService::GetAuthStatus() const {
    // LOG(INFO) << "GetAuthStatus called."; // Potentially too noisy
    if (!auth_manager_) {
        LOG(ERROR) << "GetAuthStatus called but Authentication manager is not initialized.";
        return AuthStatus::kUnknown; // Or appropriate error state
    }
    // Delegate to the authentication manager
    return auth_manager_->GetAuthStatus();
}

absl::StatusOr<UserProfile> CoreIPCService::GetCurrentUserProfile() const {
    LOG(INFO) << "GetCurrentUserProfile called.";
    if (!auth_manager_) {
        return absl::InternalError("Authentication manager is not initialized.");
    }
    // Delegate to the authentication manager
    return auth_manager_->GetCurrentUserProfile();
}

AppSettings CoreIPCService::GetAppSettings() {
    LOG(INFO) << "GetAppSettings called.";
    if (!user_settings_) {
        LOG(ERROR) << "GetAppSettings: User settings service is null.";
        return AppSettings{}; // Return default
    }
    // Delegate or construct from IUserSettings if necessary
    // For now, assume IUserSettings can provide it directly or holds the needed data.
    // This might require IUserSettings::GetAppSettings() or similar.
    // Returning default for now until IUserSettings interface is defined.
    LOG(WARNING) << "GetAppSettings returning default values (Implementation pending).";
    return user_settings_->GetAppSettings(); // Assuming this method exists
}

absl::Status CoreIPCService::SetAppSettings(const AppSettings& settings) {
    LOG(INFO) << "SetAppSettings called.";
    if (!user_settings_) {
        return absl::InternalError("SetAppSettings: User settings service is null.");
    }
    // Delegate to IUserSettings
    return user_settings_->SetAppSettings(settings); // Assuming this method exists
}

// --- Listener Management --- 
void CoreIPCService::AddStatusListener(IGameStatusListener* listener) {
    if (!listener) {
        LOG(WARNING) << "Attempted to add a null listener.";
        return;
    }
    absl::MutexLock lock(&listeners_mutex_);
    // Avoid adding duplicates
    for (const auto* existing_listener : listeners_) {
        if (existing_listener == listener) {
            LOG(INFO) << "Listener already added.";
            return; // Already added
        }
    }
    listeners_.push_back(listener);
    LOG(INFO) << "Added status listener.";

    // Immediately notify the new listener of all current game statuses
    LOG(INFO) << "Notifying new listener of current game statuses.";
    std::vector<GameStatusUpdate> current_statuses;
    {
        absl::MutexLock lock_games(&games_mutex_);
        current_statuses.reserve(current_game_statuses_.size());
        for (const auto& pair : current_game_statuses_) {
            current_statuses.push_back(pair.second);
        }
    } // Release games_mutex_ before calling listener
    
    // Check again if listener is valid before calling (might have been removed concurrently? unlikely but safe)
    // Need to re-acquire listener lock to check if it's still present before notifying
    // OR simplify: just notify without re-checking presence, listener should handle being called after removal if needed.
    // Let's stick to the simpler approach for now.
    if (!current_statuses.empty()) {
       NotifyListener(listener, current_statuses); // Pass the vector of statuses
    }
}

void CoreIPCService::RemoveStatusListener(IGameStatusListener* listener) {
    if (!listener) {
        LOG(WARNING) << "Attempted to remove a null listener.";
        return;
    }
    absl::MutexLock lock(&listeners_mutex_);
    LOG(INFO) << "Attempting to remove listener: " << listener;
    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
    if (it != listeners_.end()) {
        listeners_.erase(it);
        LOG(INFO) << "Listener removed successfully.";
    } else {
        LOG(INFO) << "Listener not found for removal.";
    }
}

// --- Internal Helpers ---

// Helper to notify all currently registered listeners
void CoreIPCService::NotifyListeners(const GameStatusUpdate& update) {
    absl::MutexLock lock(&listeners_mutex_);
    LOG(INFO) << "Notifying " << listeners_.size() << " listeners of update for game: " << update.game_id;
    for (IGameStatusListener* listener : listeners_) {
        if (listener) { // Check for null just in case
            listener->OnGameStatusUpdate(update); 
        }
    }
}

// Helper to notify a single listener (used when adding)
// Changed parameter to match the expected signature by AddStatusListener
void CoreIPCService::NotifyListener(IGameStatusListener* listener, const std::vector<GameStatusUpdate>& updates) {
     if (listener && !updates.empty()) { // Check for null listener and empty updates
        LOG(INFO) << "Notifying single listener of " << updates.size() << " game statuses.";
        for (const auto& single_update : updates) {
            listener->OnGameStatusUpdate(single_update);
        }
    }
}

// --- Background Task Monitoring ---
void CoreIPCService::MonitorTasksLoop() {
    LOG(INFO) << "Starting background task monitoring loop.";
    std::unique_lock<std::mutex> lock(monitor_mutex_);

    while (!stop_monitoring_.load(std::memory_order_acquire)) {
        // Wait for a notification or timeout
        monitor_cv_.wait_for(lock, std::chrono::seconds(5), [this] {
            return stop_monitoring_.load(std::memory_order_acquire);
        });

        if (stop_monitoring_.load(std::memory_order_acquire)) {
            break;
        }

        // Unlock temporarily to avoid holding lock while processing
        lock.unlock();

        LOG(INFO) << "Checking task statuses...";
        // TODO: Add logic here to check status of tasks in background_task_manager_
        //       and update current_game_statuses_ and notify listeners.

        // Re-lock before waiting again
        lock.lock(); 
    }

    LOG(INFO) << "Exiting background task monitoring loop.";
}

absl::StatusOr<TaskStatus> CoreIPCService::GetTaskStatus(TaskId task_id) {
    LOG(INFO) << "CoreIPCService::GetTaskStatus called for task ID: " << task_id;
    // Placeholder implementation
    return TaskStatus::kPending; // Use correct enum TaskStatus
}

absl::Status CoreIPCService::CancelTask(TaskId task_id) {
    LOG(INFO) << "CoreIPCService::CancelTask called for task ID: " << task_id;
    if (background_task_manager_) {
        background_task_manager_->RequestCancellation(task_id); // Returns void
        // Return OK status because the *request* was successfully submitted.
        return absl::OkStatus();
    } else {
        LOG(ERROR) << "CancelTask failed: Background task manager is not initialized.";
        return absl::InternalError("Background task manager not initialized");
    }
}

bool CoreIPCService::PerformIdleTasks() { // Renamed from Idle
    LOG(INFO) << "PerformIdleTasks called."; // Optional: add logging
    // Perform periodic tasks here if needed
    // Return true to keep being called, false to stop for this cycle
    // Check task statuses periodically here maybe?
    // CheckTaskStatuses(); // Factor out the check logic - Keep commented for now
    return false; // Or true depending on your logic
}

} // namespace core
} // namespace game_launcher
