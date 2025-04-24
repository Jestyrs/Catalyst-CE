// tests/core/background_task_manager_test.cpp
#include "game_launcher/core/background_task_manager.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::milliseconds
#include <atomic> // For std::atomic
#include <memory> // For std::unique_ptr

// --- Concrete Implementation for Testing --- 
// We need access to the concrete class to test it directly.
// Ideally, this would be exposed via a factory, but for now, 
// we include the source or make the class definition available.
// #include "../core/src/background_task_manager.cpp" // Not ideal, better approach needed

// Hacky way for now: Redeclare necessary parts if source isn't included
// OR ideally, move BasicBackgroundTaskManager declaration to a header if needed
// outside its cpp file (though this violates encapsulation somewhat).
// For now, assume we have a way to create it (e.g., factory function declared somewhere)

namespace game_launcher {
namespace core {

// Forward declaration or include for the concrete class if needed
class BasicBackgroundTaskManager; 

class BackgroundTaskManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<IBackgroundTaskManager> task_manager_;

    void SetUp() override {
        // Use the factory function declared in background_task_manager.h
        // and defined in background_task_manager.cpp
        task_manager_ = game_launcher::core::CreateBackgroundTaskManager(); 
        ASSERT_NE(task_manager_, nullptr) << "Failed to create Task Manager instance via factory.";
    }

    void TearDown() override {
        task_manager_.reset();
    }

    // Helper to wait for a task to reach a non-pending/running state
    std::optional<TaskInfo> WaitForTaskCompletion(TaskId task_id, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        if (!task_manager_) return std::nullopt;
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start_time < timeout) {
            auto info = task_manager_->GetTaskInfo(task_id);
            if (info &&
                info->status != TaskStatus::kPending && 
                info->status != TaskStatus::kRunning) { 
                return info;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); 
        }
        // Return last known status after timeout
        auto last_info = task_manager_->GetTaskInfo(task_id);
        // Optionally log a timeout warning
        if(last_info && (last_info->status == TaskStatus::kPending || last_info->status == TaskStatus::kRunning)) {
             // Removed gtest internal ColoredPrintf call. Use std::cout or logging framework if needed.
             // std::cout << "[ WARN ] Timeout waiting for task " << task_id << " to complete." << std::endl;
        }
        return last_info;
    }
};

// Placeholder test - needs manager instantiation to work
TEST_F(BackgroundTaskManagerTest, StartSimpleTaskAndSucceeds) {
    ASSERT_NE(task_manager_, nullptr);

    std::atomic<bool> task_executed = false;
    auto work = [&](ProgressReporter progress_reporter) -> bool {
        progress_reporter(50.0f, "Halfway");
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
        task_executed = true; 
        progress_reporter(100.0f, "Done");
        return true; 
    };

    TaskId task_id = task_manager_->StartTask(work, "Simple Test Task");

    auto initial_info = task_manager_->GetTaskInfo(task_id);
    ASSERT_TRUE(initial_info.has_value());
    EXPECT_EQ(initial_info->id, task_id);
    // Status might be kPending or kRunning immediately after start
    EXPECT_TRUE(initial_info->status == TaskStatus::kPending || initial_info->status == TaskStatus::kRunning); 
    EXPECT_EQ(initial_info->description, "Simple Test Task");

    auto final_info = WaitForTaskCompletion(task_id);
    ASSERT_TRUE(final_info.has_value()) << "Task info should exist after waiting.";
    EXPECT_EQ(final_info->status, TaskStatus::kSucceeded);
    EXPECT_EQ(final_info->description, "Done"); 
    EXPECT_TRUE(task_executed.load());
}

// Test that a task throwing an exception is marked as Failed
TEST_F(BackgroundTaskManagerTest, StartTaskAndFails) {
    ASSERT_NE(task_manager_, nullptr);

    // Define a task that throws an exception
    auto lambda_that_throws = [](ProgressReporter update_progress_fn) -> bool {
        update_progress_fn(50.0f, "About to fail..."); 
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
        throw std::runtime_error("Simulated task failure");
        // return false; 
    };

    TaskId task_id = task_manager_->StartTask(
        lambda_that_throws, // Let StartTask handle the conversion if possible
        "Failing Task"
    );

    // Wait for the task to complete (fail) using the helper function
    auto final_info = WaitForTaskCompletion(task_id);

    // Check the final status
    ASSERT_TRUE(final_info.has_value()) << "Task info should exist after waiting for failure.";
    const auto expected_status = TaskStatus::kFailed;
    const auto unexpected_status = TaskStatus::kSucceeded;
    EXPECT_EQ(final_info->status, expected_status);

    // Ensure it didn't somehow succeed
    ASSERT_NE(final_info->status, unexpected_status) << "Task unexpectedly succeeded.";

    // Optionally, check if the description contains failure info (current impl might not)
    // EXPECT_NE(final_info->description.find("failed"), std::string::npos);
}

// Test that a task can be cancelled while running
TEST_F(BackgroundTaskManagerTest, RequestCancellation) {
    ASSERT_NE(task_manager_, nullptr);

    std::atomic<bool> task_started = false;
    std::atomic<bool> cancellation_checked_during_run = false;

    // Define a task that runs for a bit and can be cancelled
    auto cancellable_work = [&](ProgressReporter reporter) -> bool {
        task_started = true;
        reporter(10.0f, "Starting cancellable task...");
        // Simulate some work, checking for cancellation periodically
        for (int i = 0; i < 5; ++i) {
            // In a real task, cancellation check would be integrated into the work loop
            // We rely on the BackgroundTaskManager's internal check after the reporter call
            // and the check within the wrapper lambda.
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            // If the worker function *itself* could detect cancellation via ProgressReporter
            // (e.g., if ProgressReporter threw an exception or returned a special value),
            // we could test that here. Our current ProgressReporter doesn't support this.
            cancellation_checked_during_run = true; // Mark that we reached here
            reporter(20.0f + (i * 15.0f), "Working...");
        }
        reporter(100.0f, "Finished work (should be cancelled before this)");
        return true; // Return success if not cancelled by the manager
    };

    TaskId task_id = task_manager_->StartTask(cancellable_work, "Cancellable Task");

    // Wait a brief moment to ensure the task likely started
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    ASSERT_TRUE(task_started.load()) << "Task should have started.";

    // Request cancellation while the task is presumably running
    task_manager_->RequestCancellation(task_id);
    std::cout << "Cancellation requested for task " << task_id << std::endl;

    // Wait for the task to complete using the helper function
    auto final_info = WaitForTaskCompletion(task_id, std::chrono::seconds(2)); // Shorter timeout maybe

    // Check the final status
    ASSERT_TRUE(final_info.has_value()) << "Task info should exist after waiting for cancellation.";
    EXPECT_EQ(final_info->status, TaskStatus::kCancelled);
    EXPECT_TRUE(cancellation_checked_during_run) << "Work loop should have run at least once.";
}

// Test handling multiple concurrent tasks
TEST_F(BackgroundTaskManagerTest, MultipleConcurrentTasks) {
    ASSERT_NE(task_manager_, nullptr);
    const int num_tasks = 5;
    std::vector<TaskId> task_ids;
    std::atomic<int> completed_count{0};

    // Define a simple task that sleeps for a bit
    auto simple_work = [&](int task_num, ProgressReporter reporter) -> bool {
        reporter(0.0f, "Starting task...");
        std::this_thread::sleep_for(std::chrono::milliseconds(50 + (task_num * 10))); // Vary sleep time slightly
        reporter(100.0f, "Task finished.");
        completed_count++;
        return true;
    };

    // Start multiple tasks
    for (int i = 0; i < num_tasks; ++i) {
        std::string desc = "Concurrent Task " + std::to_string(i);
        // Need to capture 'i' by value for the lambda
        task_ids.push_back(task_manager_->StartTask([i, &simple_work](ProgressReporter reporter) {
            return simple_work(i, reporter);
        }, desc));
    }

    EXPECT_EQ(task_ids.size(), num_tasks);

    // Wait for all tasks to complete
    std::vector<TaskInfo> final_infos;
    for (TaskId id : task_ids) {
        auto info = WaitForTaskCompletion(id, std::chrono::seconds(5)); // Use generous timeout
        ASSERT_TRUE(info.has_value()) << "Task info missing for task " << id;
        final_infos.push_back(*info);
    }

    // Check final statuses
    EXPECT_EQ(final_infos.size(), num_tasks);
    EXPECT_EQ(completed_count.load(), num_tasks) << "Not all task lambdas seem to have completed.";

    for (const auto& info : final_infos) {
        EXPECT_EQ(info.status, TaskStatus::kSucceeded) << "Task " << info.id << " did not succeed.";
        EXPECT_EQ(info.progress_percentage, 100.0f);
        // Check that the final description reported is the one set at the end of the task work.
        EXPECT_EQ(info.description, "Task finished.");
        // We check the initial description was set correctly via GetActiveTasks if needed.
    }
}

// Test progress reporting updates
TEST_F(BackgroundTaskManagerTest, ProgressReporting) {
    ASSERT_NE(task_manager_, nullptr);

    // Define the expected progress points and descriptions
    std::map<float, std::string> expected_progress = {
        {0.0f, "Starting..."},
        {25.0f, "Step 1/3"},
        {50.0f, "Step 2/3"},
        {75.0f, "Step 3/3"},
        {100.0f, "Complete!"}
    };

    // Define the task work function
    auto work = [&expected_progress](ProgressReporter reporter) -> bool {
        reporter(0.0f, expected_progress[0.0f]);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        reporter(25.0f, expected_progress[25.0f]);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        reporter(50.0f, expected_progress[50.0f]);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        reporter(75.0f, expected_progress[75.0f]);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        reporter(100.0f, expected_progress[100.0f]);
        return true;
    };

    // Start the task
    TaskId task_id = task_manager_->StartTask(work, "Initial Description");

    // Poll for progress updates and store observed states
    std::map<float, std::string> observed_progress;
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(2); // Generous timeout for polling
    TaskStatus final_status = TaskStatus::kPending;

    while (std::chrono::steady_clock::now() - start_time < timeout) {
        auto info_opt = task_manager_->GetTaskInfo(task_id);
        if (info_opt) {
            // Record the latest observed state for this percentage
            observed_progress[info_opt->progress_percentage] = info_opt->description;
            final_status = info_opt->status;
            // Stop polling if task has reached a terminal state
            if (final_status == TaskStatus::kSucceeded || 
                final_status == TaskStatus::kFailed || 
                final_status == TaskStatus::kCancelled) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Polling interval
    }

    // Assert that all expected intermediate progress points were observed
    // We check percentages < 100% here, 100% is checked with the final state.
    EXPECT_TRUE(observed_progress.count(0.0f));
    EXPECT_EQ(observed_progress[0.0f], expected_progress[0.0f]);
    EXPECT_TRUE(observed_progress.count(25.0f));
    EXPECT_EQ(observed_progress[25.0f], expected_progress[25.0f]);
    EXPECT_TRUE(observed_progress.count(50.0f));
    EXPECT_EQ(observed_progress[50.0f], expected_progress[50.0f]);
    EXPECT_TRUE(observed_progress.count(75.0f));
    EXPECT_EQ(observed_progress[75.0f], expected_progress[75.0f]);

    // Assert the final state after polling loop (or use WaitForTaskCompletion for robustness)
    auto final_info_opt = WaitForTaskCompletion(task_id, std::chrono::seconds(1)); // Should be quick
    ASSERT_TRUE(final_info_opt.has_value());

    EXPECT_EQ(final_info_opt->status, TaskStatus::kSucceeded);
    EXPECT_EQ(final_info_opt->progress_percentage, 100.0f);
    EXPECT_EQ(final_info_opt->description, expected_progress[100.0f]);
}

  // TODO: Add more tests:
  // - Task Cancellation (Requesting cancellation, checking status) // Done
  // - Multiple tasks (Ensure they run concurrently, check status individually) // Done
  // - Progress reporting (Verify progress updates are correctly reflected) // Done
 
 } // namespace core
 } // namespace game_launcher
