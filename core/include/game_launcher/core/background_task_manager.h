// core/include/game_launcher/core/background_task_manager.h
#ifndef GAME_LAUNCHER_CORE_BACKGROUND_TASK_MANAGER_H_
#define GAME_LAUNCHER_CORE_BACKGROUND_TASK_MANAGER_H_

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint> // For uint64_t

namespace game_launcher {
namespace core {

// Represents the unique identifier for a background task.
using TaskId = uint64_t;

// Represents the status of a background task.
enum class TaskStatus {
  kPending,
  kRunning,
  kPaused,    // Optional: For tasks that can be paused/resumed
  kSucceeded,
  kFailed,
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

// Type definition for the work function a background task will execute.
// Takes a function to report progress (percentage, description).
// Returns true on success, false on failure.
using TaskWorkFunction = std::function<bool(std::function<void(float, std::string_view)>)>;

// Interface for managing background tasks.
class IBackgroundTaskManager {
 public:
  virtual ~IBackgroundTaskManager() = default;

  // Starts a new background task.
  // task_work: The function representing the work to be done.
  // initial_description: A user-friendly description for the task initially.
  // Returns a unique TaskId for the newly created task.
  virtual TaskId StartTask(TaskWorkFunction task_work, std::string_view initial_description) = 0;

  // Retrieves the current status and progress of a specific task.
  // Returns std::nullopt if the task_id is invalid.
  virtual std::optional<TaskInfo> GetTaskInfo(TaskId task_id) const = 0;

  // Retrieves information for all currently active (Pending, Running, Paused) tasks.
  virtual std::vector<TaskInfo> GetActiveTasks() const = 0;

  // Attempts to cancel a running or pending task.
  // Cancellation might not be immediate or guaranteed depending on the task.
  // Returns true if the cancellation request was initiated, false otherwise (e.g., task not found or already completed).
  virtual bool CancelTask(TaskId task_id) = 0;

  // TODO: Consider adding methods for pausing/resuming tasks if needed.
  // TODO: Consider mechanisms for task prioritization.
  // TODO: Consider adding callback/event mechanisms for task completion/progress updates.
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_BACKGROUND_TASK_MANAGER_H_
