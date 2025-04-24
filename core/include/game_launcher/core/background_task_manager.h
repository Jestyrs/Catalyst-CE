// core/include/game_launcher/core/background_task_manager.h
#ifndef GAME_LAUNCHER_CORE_BACKGROUND_TASK_MANAGER_H_
#define GAME_LAUNCHER_CORE_BACKGROUND_TASK_MANAGER_H_

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint> // For uint64_t
#include <memory> // For std::unique_ptr

namespace game_launcher {
namespace core {

// Represents the unique identifier for a background task.
using TaskId = uint64_t;

// Constant representing an invalid or unassigned task ID.
constexpr TaskId kInvalidTaskId = 0;

// Represents the status of a background task.
enum class TaskStatus {
  kPending,
  kRunning,
  kPaused,    // Optional: For tasks that can be paused/resumed
  kSucceeded,   // Renamed from Succeeded
  kFailed,      // Renamed from Failed
  kCancelled
};

// Structure holding information about a background task's progress and status.
struct TaskInfo {
  TaskId id;
  TaskStatus status;
  float progress_percentage; // 0.0f to 100.0f
  std::string description;   // User-facing description (e.g., "Downloading Game X", "Verifying files...")
  std::optional<std::string> error_message; // Set if status is kFailed
};

// Using alias for clarity as suggested
using ProgressReporter = std::function<void(float percentage, std::string_view description)>;
using TaskWork = std::function<bool(ProgressReporter progress_reporter)>; // Returns true on success, false on explicit failure

// Interface for managing background tasks.
class IBackgroundTaskManager {
 public:
  virtual ~IBackgroundTaskManager() = default;

  // Starts a new background task.
  // task_work: The function representing the work to be done.
  // initial_description: A user-friendly description for the task initially.
  // Returns a unique TaskId for the newly created task.
  virtual TaskId StartTask(TaskWork work, std::string_view initial_description) = 0;

  // Retrieves the current status and progress of a specific task.
  // Returns std::nullopt if the task_id is invalid.
  virtual std::optional<TaskInfo> GetTaskInfo(TaskId task_id) const = 0;

  // Retrieves information for all currently active (Pending, Running, Paused) tasks.
  virtual std::vector<TaskInfo> GetActiveTasks() const = 0;

  // Renamed for clarity based on suggestion (although not strictly required by the fix)
  virtual void RequestCancellation(TaskId task_id) = 0;

  // TODO: Consider adding methods for pausing/resuming tasks if needed.
  // TODO: Consider mechanisms for task prioritization.
  // TODO: Consider adding callback/event mechanisms for task completion/progress updates.
};

// Factory function to create the default background task manager implementation.
std::unique_ptr<IBackgroundTaskManager> CreateBackgroundTaskManager();

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_BACKGROUND_TASK_MANAGER_H_
