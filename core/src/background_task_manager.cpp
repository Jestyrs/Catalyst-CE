// core/src/background_task_manager.cpp
#include "game_launcher/core/background_task_manager.h"

#include <future>        // For std::async, std::future, std::launch
#include <mutex>         // For std::mutex, std::lock_guard
#include <unordered_map>
#include <atomic>        // For std::atomic<TaskId>, std::atomic<bool>
#include <vector>
#include <thread>        // For std::this_thread::sleep_for
#include <chrono>        // For std::chrono::milliseconds
#include <iostream>      // For temporary logging
#include <memory>        // For std::shared_ptr, std::make_shared

namespace game_launcher {
namespace core {

// Internal structure to hold task details and state
struct TaskInternal {
    TaskInfo info;
    std::atomic<bool> cancellation_requested{false};
    std::future<bool> future;         // Holds the result of the async operation

    // We need a way to update progress from the worker thread safely
    mutable std::mutex progress_mutex; // <--- Add 'mutable' here
    float current_progress = 0.0f;
    std::string current_description;

    // Progress update function for the worker thread
    void UpdateProgress(float percentage, std::string_view description) {
        std::lock_guard<std::mutex> lock(progress_mutex);
        current_progress = percentage;
        current_description = description;
        // Note: A real implementation might use a queue or condition variable
        // to notify the main thread or UI layer instead of just storing here.
    }
};

// Basic implementation using std::async and mutexes for state management.
class BasicBackgroundTaskManager : public IBackgroundTaskManager {
 public:
  BasicBackgroundTaskManager() : next_task_id_(1) {} // Start IDs from 1
  ~BasicBackgroundTaskManager() override {
      // Ensure all threads are joined or detached cleanly if necessary.
      // std::async futures usually handle this, but explicit management
      // might be needed for more complex thread pools.
      // For simplicity, we assume tasks complete or are simple enough.
      std::cout << "BasicBackgroundTaskManager: Shutting down." << std::endl;
      // A real implementation might try to cancel pending tasks here.
  }

  // Non-copyable/movable
  BasicBackgroundTaskManager(const BasicBackgroundTaskManager&) = delete;
  BasicBackgroundTaskManager& operator=(const BasicBackgroundTaskManager&) = delete;
  BasicBackgroundTaskManager(BasicBackgroundTaskManager&&) = delete;
  BasicBackgroundTaskManager& operator=(BasicBackgroundTaskManager&&) = delete;

  TaskId StartTask(TaskWork work, std::string_view initial_description) override {
    TaskId current_id = next_task_id_++; // Atomically get the next ID

    auto task_internal = std::make_shared<TaskInternal>();
    task_internal->info.id = current_id;
    task_internal->info.status = TaskStatus::kPending;
    task_internal->info.progress_percentage = 0.0f;
    task_internal->info.description = initial_description;
    task_internal->current_description = initial_description; // Initialize internal description


    // Lambda to wrap the user's work function and manage state
    auto task_wrapper = [this, task_internal, work]() -> bool {
        // --- Update status to Running ---
        {
             std::lock_guard<std::mutex> lock(tasks_mutex_);
             auto it = tasks_.find(task_internal->info.id);
             if (it != tasks_.end()) {
                 it->second->info.status = TaskStatus::kRunning;
             } else {
                 // Should not happen if started correctly
                 std::cerr << "Error: Task " << task_internal->info.id << " not found at start!" << std::endl;
                 return false;
             }
        }
        std::cout << "Task " << task_internal->info.id << ": Started (" << task_internal->info.description << ")" << std::endl;


        // --- Execute the actual work ---
        bool success = false;
        std::string error_msg;
        try {
            // Provide the progress update function to the worker
            success = work([task_internal](float percentage, std::string_view description) {
                task_internal->UpdateProgress(percentage, description);
                // Check for cancellation periodically within the work function if possible
                if (task_internal->cancellation_requested.load()) {
                    // Indicate cancellation requested - the worker function should handle this
                    // Ideally, throw an exception or return a specific value
                    std::cout << "Task " << task_internal->info.id << ": Cancellation detected by worker." << std::endl;
                    // For simplicity, we'll just return false here, the worker ideally checks this
                }
            });
        } catch (const std::exception& e) {
             std::cerr << "Task " << task_internal->info.id << ": Exception caught: " << e.what() << std::endl;
             success = false;
             error_msg = e.what();
        } catch (...) {
             std::cerr << "Task " << task_internal->info.id << ": Unknown exception caught." << std::endl;
             success = false;
             error_msg = "Unknown error";
        }

        // --- Update final status ---
        {
             std::lock_guard<std::mutex> lock(tasks_mutex_);
             auto it = tasks_.find(task_internal->info.id);
             if (it != tasks_.end()) {
                 if (task_internal->cancellation_requested.load()) {
                     it->second->info.status = TaskStatus::kCancelled;
                     std::cout << "Task " << task_internal->info.id << ": Marked as Cancelled." << std::endl;
                 } else if (success) {
                     it->second->info.status = TaskStatus::kSucceeded;
                     it->second->info.progress_percentage = 100.0f; // Ensure completion progress
                     std::cout << "Task " << task_internal->info.id << ": Marked as Succeeded." << std::endl;
                 } else {
                     it->second->info.status = TaskStatus::kFailed;
                     it->second->info.error_message = error_msg.empty() ? "Task failed" : error_msg;
                     std::cout << "Task " << task_internal->info.id << ": Marked as Failed. Error: " << it->second->info.error_message.value_or("N/A") << std::endl;
                 }
                 // Update description one last time from internal state if needed
                 it->second->info.description = task_internal->current_description;
             }
        }
        return success; // Return the success status
    };

    // Launch the task asynchronously using std::async
    // std::launch::async ensures it runs on a potentially new thread
    task_internal->future = std::async(std::launch::async, task_wrapper);

    // Store the task details - must be done *before* the task might complete
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        tasks_[current_id] = task_internal;
    }
     std::cout << "BasicBackgroundTaskManager: Task " << current_id << " queued." << std::endl;


    return current_id;
  }

  std::optional<TaskInfo> GetTaskInfo(TaskId task_id) const override {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
      return std::nullopt; // Task not found
    }

    auto& task_internal = it->second;
    TaskInfo info_copy = task_internal->info; // Create a copy to return

    // Update the copy with the latest progress/description from internal state
    {
        std::lock_guard<std::mutex> progress_lock(task_internal->progress_mutex); // OK: progress_mutex is mutable
        info_copy.progress_percentage = task_internal->current_progress;
        info_copy.description = task_internal->current_description;
    }

    // Check if the task has finished (without blocking indefinitely)
    // This is a simplified check. A robust system might need callbacks or polling.
    if (task_internal->info.status == TaskStatus::kPending || task_internal->info.status == TaskStatus::kRunning) {
         // Use wait_for with zero duration to check if the future is ready
        if (task_internal->future.valid() &&
            task_internal->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            // The task likely finished between the wrapper setting the status and this check.
            // Re-read the status which should have been updated by the wrapper lambda.
            // Note: This doesn't re-trigger the lambda, just gets the status potentially set by it.
            // The future's result isn't explicitly retrieved here to avoid blocking if somehow
            // it wasn't *quite* ready, but the internal status should be authoritative.
            std::cout << "Task " << task_id << ": Detected completion via future status." << std::endl;
            // Status in info_copy might be slightly stale if the task just finished,
            // but the wrapper lambda should have updated the internal info.status.
            // We return the copy which has the latest progress/description and potentially
            // slightly stale status if completion happened between copy and return.
            // A more complex system might re-fetch status here.
        }
    }

    return info_copy; // Return the updated copy
  }

  std::vector<TaskInfo> GetActiveTasks() const override {
    std::vector<TaskInfo> active_tasks;
    std::lock_guard<std::mutex> lock(tasks_mutex_); // tasks_mutex_ is mutable

    // std::vector<TaskId> completed_task_ids; // Store IDs of tasks to potentially clean up

    for (auto const& [id, task_internal_ptr] : tasks_) {
        const auto& task_internal = *task_internal_ptr; // Dereference shared_ptr

        TaskInfo info_copy = task_internal.info; // Create a copy for potential inclusion

        // Update the copy with the latest progress/description from internal state
         {
            std::lock_guard<std::mutex> progress_lock(task_internal.progress_mutex); // OK: progress_mutex is mutable
            info_copy.progress_percentage = task_internal.current_progress;
            info_copy.description = task_internal.current_description;
         }

        TaskStatus current_status = info_copy.status; // Use status from the copy

        // Non-blocking check if a running/pending task's future is ready
        if ((current_status == TaskStatus::kPending || current_status == TaskStatus::kRunning) &&
            task_internal.future.valid() &&
            task_internal.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
             std::cout << "Task " << id << ": Detected completion during GetActiveTasks scan." << std::endl;
             // We rely on the stored status being correct. Update the copy's status if needed.
             // Note: The internal info.status might have been updated by the wrapper.
             // We read it again here to update our copy if the task just finished.
             current_status = task_internal.info.status; // Re-read potentially updated status
             info_copy.status = current_status; // Update the copy's status
        }


        if (current_status == TaskStatus::kPending ||
            current_status == TaskStatus::kRunning ||
            current_status == TaskStatus::kPaused) {
          active_tasks.push_back(info_copy); // Add the updated copy
        } else {
            // Optionally, mark completed tasks for cleanup later
            // completed_task_ids.push_back(id);
        }
    }

    // Optional: Cleanup completed tasks from the main map
    // Be careful with iterator invalidation if removing while iterating.
    // for(TaskId id_to_remove : completed_task_ids) {
    //     tasks_.erase(id_to_remove);
    // }

    return active_tasks;
  }

  void RequestCancellation(TaskId task_id) override {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        if (it->second->info.status == TaskStatus::kPending || it->second->info.status == TaskStatus::kRunning) {
            it->second->cancellation_requested.store(true);
            std::cout << "Task " << task_id << ": Cancellation requested." << std::endl;
        } else {
            std::cout << "Task " << task_id << ": Cannot request cancellation, task not pending/running." << std::endl;
        }
    } else {
         std::cout << "Task " << task_id << ": Cannot request cancellation, task not found." << std::endl;
    }
  }

 private:
  mutable std::mutex tasks_mutex_; // Mutex to protect access to the tasks map
  std::unordered_map<TaskId, std::shared_ptr<TaskInternal>> tasks_; // Map of TaskId to internal task details
  std::atomic<TaskId> next_task_id_; // Atomically incrementing task ID counter
};

std::unique_ptr<IBackgroundTaskManager> CreateBackgroundTaskManager() {
    return std::make_unique<BasicBackgroundTaskManager>();
}

} // namespace core
} // namespace game_launcher
