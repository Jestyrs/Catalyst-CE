// 1. Standard Library Headers
#include <filesystem> // Often needs to be early
#include <fstream>
#include <functional>
#include <map>       // For std::map
#include <memory>
#include <mutex>     // For std::mutex
#include <string>
#include <system_error> // For std::error_code
#include <thread>    // For std::this_thread
#include <chrono>    // For std::chrono
#include <vector>
#include <iterator>  // For std::istreambuf_iterator
#include <nlohmann/json.hpp> // Added for JSON operations

// 2. Platform-Specific Headers
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h> // For _T macro if used (though not strictly needed here)
#include <processthreadsapi.h> // Explicit include for CreateProcessW
#include <winbase.h> // For STARTUPINFOW, PROCESS_INFORMATION
#endif // _WIN32

// 3. Third-Party Library Headers
#include "absl/cleanup/cleanup.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h" // For StrFormat used in errors
#include "absl/strings/match.h" // Added for EqualsIgnoreCase
#include "absl/synchronization/mutex.h"
#pragma warning(push)
#pragma warning(disable : 4244) // Disable truncation warning from picosha2 internals
#include "picosha2.h" // Added for hashing (assuming header name)
#pragma warning(pop)

// 4. Project Headers (Dependencies)
#include "game_launcher/core/background_task_manager.h" // Include for TaskController, TaskId
#include "game_launcher/core/game_management_service.h" // Defines GameInfo, IGameManagementService
#include "game_launcher/core/game_status.h" // Defines GameState
#include "game_launcher/core/user_settings.h" // Include for IUserSettings
// #include "game_launcher/core/utils/string_utils.h" // File not found, seems unused

// 5. This File's Header
#include "game_launcher/core/basic_game_management_service.h"

// Helper function definitions (need to be after includes)
namespace {
// Platform-specific helper needs visibility of windows.h / other platform headers
#ifdef _WIN32
// Converts a UTF-8 encoded std::string to a std::wstring (Windows-specific)
std::wstring Utf8ToWstring(const std::string& utf8_string) {
    if (utf8_string.empty()) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), (int)utf8_string.size(), NULL, 0);
    if (size_needed == 0) {
        // Handle error, e.g., log or throw
        LOG(ERROR) << "MultiByteToWideChar failed to calculate size, error code: " << static_cast<unsigned long>(GetLastError());
        return std::wstring(); // Or throw an exception
    }
    std::wstring wide_string(size_needed, 0);
    int chars_converted = MultiByteToWideChar(CP_UTF8, 0, utf8_string.c_str(), (int)utf8_string.size(), &wide_string[0], size_needed);
    if (chars_converted == 0) {
        // Handle error
        LOG(ERROR) << "MultiByteToWideChar failed to convert string, error code: " << static_cast<unsigned long>(GetLastError());
        return std::wstring(); // Or throw an exception
    }
    return wide_string;
}
#endif // _WIN32

} // anonymous namespace

// Filesystem and JSON namespace aliases
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {
// Placeholder struct for download tasks
struct DownloadTask {
    std::string url;
    fs::path destination;
    std::string expected_hash;
};
} // anonymous namespace

namespace game_launcher {
namespace core {

// Declaration for DownloadString
absl::StatusOr<std::string> DownloadString(const std::string& url);

BasicGameManagementService::BasicGameManagementService(const std::filesystem::path& games_json_path,
                                                       std::shared_ptr<IUserSettings> user_settings,
                                                       IBackgroundTaskManager* background_task_manager)
    : user_settings_(std::move(user_settings)), 
      background_task_manager_(background_task_manager) { // Initialize the background task manager pointer
    absl::Status load_status = LoadGamesFromFile(games_json_path);
    if (!load_status.ok()) {
        LOG(WARNING) << "Failed to load game data from " 
                     << games_json_path.string() << ": " << load_status;
        // Decide if this should be a fatal error. For now, log and continue.
    }
}

// Helper to load game data from the JSON file.
absl::Status BasicGameManagementService::LoadGamesFromFile(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        return absl::NotFoundError(absl::StrCat("Game data file not found: ", file_path.string()));
    }

    std::ifstream file_stream(file_path);
    if (!file_stream.is_open()) {
        return absl::InternalError(absl::StrCat("Failed to open game data file: ", file_path.string()));
    }

    // Read the entire file into a string first
    std::string file_content((std::istreambuf_iterator<char>(file_stream)),
                             std::istreambuf_iterator<char>());
    
    // Close the stream explicitly (optional, as it closes on destruction)
    file_stream.close();

    json data;
    try {
        // Parse the string content
        data = json::parse(file_content);
    } catch (json::parse_error& e) {
        return absl::InvalidArgumentError(absl::StrCat("Failed to parse game data JSON: ", e.what()));
    }

    if (!data.contains("games") || !data["games"].is_array()) {
        return absl::InvalidArgumentError("Game data JSON missing 'games' array.");
    }

    loaded_games_.clear(); // Clear any previously loaded data
    for (const auto& game_json : data["games"]) {
        if (!game_json.is_object()) {
            LOG(WARNING) << "Skipping non-object entry in games array.";
            continue;
        }

        try {
            GameInfo game_info;
            game_info.id = game_json.value("id", "");
            game_info.name = game_json.value("name", "");
            game_info.install_path = game_json.value("install_path", "");
            game_info.executable_path = game_json.value("executable_path", "");
            game_info.version = game_json.value("version", "");

            if (loaded_games_.contains(game_info.id)) {
                 LOG(WARNING) << "Duplicate game ID found, overwriting: " << game_info.id;
            }
            loaded_games_[game_info.id] = game_info;

        } catch (json::out_of_range& e) {
            LOG(WARNING) << "Skipping game entry due to missing field: " << e.what();
        } catch (json::type_error& e) {
            LOG(WARNING) << "Skipping game entry due to incorrect field type: " << e.what();
        }
    }
    LOG(INFO) << "Loaded " << loaded_games_.size() << " games from " << file_path.string();
    return absl::OkStatus();
}

// Marked const - reads loaded_games_
absl::StatusOr<std::vector<GameInfo>> BasicGameManagementService::GetInstalledGames() const {
    std::vector<GameInfo> games_list;
    // Ensure map access is const-correct
    games_list.reserve(loaded_games_.size());
    for (const auto& pair : loaded_games_) {
        // TODO: Filter based on actual installation status (e.g., check local manifest)
        // For now, return all games loaded from the config file.
        games_list.push_back(pair.second);
    }
    return games_list;
}

// Marked const - reads loaded_games_
absl::StatusOr<GameInfo> BasicGameManagementService::GetGameDetails(std::string_view game_id) const {
    std::string id_str(game_id); // Convert string_view to string for map lookup
    auto it = loaded_games_.find(id_str);
    if (it != loaded_games_.end()) {
        // Return a copy of the GameInfo
        return it->second;
    }
    LOG(WARNING) << "GetGameDetails failed to find game ID: " << game_id;
    return absl::NotFoundError(absl::StrCat("Game not found with ID: ", game_id));
}

absl::Status BasicGameManagementService::LaunchGame(std::string_view game_id) {
#ifndef _WIN32
    // Placeholder/Error for non-Windows platforms
    LOG(ERROR) << "LaunchGame is currently only implemented for Windows.";
    return absl::UnimplementedError("LaunchGame is only implemented for Windows.");
#else
    std::string game_id_str(game_id);
    LOG(INFO) << "Attempting to launch game: " << game_id_str;

    // 1. Get Game Details
    // Note: Assumes game data is already loaded into loaded_games_
    // A lock might be needed if game data can change concurrently.
    auto it = loaded_games_.find(game_id_str);
    if (it == loaded_games_.end()) {
        LOG(ERROR) << "Game ID not found: " << game_id_str;
        return absl::NotFoundError(absl::StrFormat("Game ID '%s' not found.", game_id_str));
    }
    const GameInfo& game_info = it->second;

    // 2. Validate Paths
    if (game_info.install_path.empty() || game_info.executable_path.empty()) {
        LOG(ERROR) << "Game '" << game_id_str << "' has missing install or executable path information.";
        return absl::FailedPreconditionError(absl::StrFormat("Game '%s' is missing path information.", game_id_str));
    }

    std::filesystem::path install_dir;
    std::filesystem::path executable_rel_path;
    std::filesystem::path full_executable_path;
    try {
        install_dir = game_info.install_path;
        executable_rel_path = game_info.executable_path;
        // Ensure paths are treated correctly, especially if relative
        full_executable_path = install_dir / executable_rel_path;
        full_executable_path = std::filesystem::absolute(full_executable_path);
         install_dir = std::filesystem::absolute(install_dir);
    } catch (const std::filesystem::filesystem_error& e) {
        LOG(ERROR) << "Filesystem error constructing path for game '" << game_id_str << "': " << e.what();
        return absl::InternalError(absl::StrFormat("Filesystem error for game '%s': %s", game_id_str, e.what()));
    }
    

    std::error_code ec;
    if (!std::filesystem::exists(full_executable_path, ec) || ec) {
         LOG(ERROR) << "Executable file not found or error checking existence for game '" << game_id_str
                       << "' at path: " << full_executable_path.string() << ". Error: " << ec.message();
        return absl::NotFoundError(absl::StrFormat("Executable for game '%s' not found at '%s'. Error: %s", 
                                             game_id_str, full_executable_path.string(), ec.message()));
    }
     if (!std::filesystem::is_regular_file(full_executable_path, ec) || ec) {
         LOG(ERROR) << "Executable path does not point to a regular file for game '" << game_id_str
                       << "' at path: " << full_executable_path.string() << ". Error: " << ec.message();
        return absl::InvalidArgumentError(absl::StrFormat("Executable path for game '%s' is not a file: '%s'. Error: %s", 
                                                game_id_str, full_executable_path.string(), ec.message()));
    }

    // 3. Prepare for CreateProcessW
    std::wstring w_executable_path = Utf8ToWstring(full_executable_path.string());
    // Quote path in case of spaces, use this as the command line argument
    std::wstring w_command_line = L"\"" + w_executable_path + L"\"";
    std::wstring w_working_directory = Utf8ToWstring(install_dir.string());

    if (w_executable_path.empty() || w_working_directory.empty()) {
        LOG(ERROR) << "Failed to convert paths to WString for game '" << game_id_str << "'. Check logs for MultiByteToWideChar errors.";
        return absl::InternalError(absl::StrFormat("Path conversion failed for game '%s'.", game_id_str));
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    LOG(INFO) << "Launching process: " << full_executable_path.string();
    LOG(INFO) << "Working directory: " << install_dir.string();

    // 4. Launch Process
    BOOL success = CreateProcessW(
        NULL,                   // lpApplicationName - Use lpCommandLine
        &w_command_line[0],     // lpCommandLine - Mutable wide string buffer
        NULL,                   // lpProcessAttributes
        NULL,                   // lpThreadAttributes
        FALSE,                  // bInheritHandles
        CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS, // dwCreationFlags
        NULL,                   // lpEnvironment
        w_working_directory.c_str(), // lpCurrentDirectory
        &si,                    // lpStartupInfo
        &pi                     // lpProcessInformation
    );

    // 5. Check Result & Cleanup
    if (!success) {
        DWORD error_code = GetLastError();
        LOG(ERROR) << "Failed to create process for game '" << game_id_str << "'. Error code: " << static_cast<unsigned long>(error_code);
        return absl::InternalError(absl::StrFormat("Failed to launch game '%s'. WinAPI Error: %d", game_id_str, error_code));
    } else {
        LOG(INFO) << "Successfully launched game '" << game_id_str << "' with Process ID: " << pi.dwProcessId;
        // Close process and thread handles immediately as we don't need them.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return absl::OkStatus();
    }
#endif // _WIN32
}

// Stub implementation for UninstallGame
absl::Status BasicGameManagementService::UninstallGame(std::string_view game_id) {
    LOG(WARNING) << "UninstallGame function is not yet implemented for game: " << game_id;
    return absl::UnimplementedError("UninstallGame is not implemented");
}

// Stub implementation for DownloadFile (private helper)
absl::Status BasicGameManagementService::DownloadFile(const std::string& url,
                                                  const std::filesystem::path& destination_path) {
    LOG(WARNING) << "DownloadFile function is not yet implemented. URL: " << url 
                 << ", Destination: " << destination_path.string();
    return absl::UnimplementedError("DownloadFile is not implemented");
}

absl::Status BasicGameManagementService::CancelOperation(std::string_view game_id) {
    std::string game_id_str(game_id);
    LOG(INFO) << "Received request to cancel operation for game: " << game_id_str;

    TaskId task_to_cancel{}; // Initialize to default value
    bool found = false;

    {
        absl::MutexLock lock(&active_operations_mutex_);
        auto it = active_operations_.find(game_id_str);
        if (it != active_operations_.end()) {
            task_to_cancel = it->second;
            found = true;
            // Remove the operation immediately upon cancellation request.
            // The background task itself should handle the cancellation signal gracefully.
            active_operations_.erase(it);
        } else {
            LOG(WARNING) << "CancelOperation called for game ID '" << game_id_str 
                         << "' but no active operation found.";
        }
    }

    if (found) {
        LOG(INFO) << "Requesting cancellation for task ID: " << task_to_cancel;
        background_task_manager_->RequestCancellation(task_to_cancel);
        // Consider if the status should reflect whether cancellation is guaranteed
        // or just requested. OkStatus implies the request was successfully sent.
        return absl::OkStatus(); 
    } else {
        return absl::NotFoundError(absl::StrCat("No active operation found for game: ", game_id_str));
    }
}

// Implementations for AddStatusListener and RemoveStatusListener removed.

absl::Status BasicGameManagementService::InstallGame(std::string_view game_id, 
                                                     std::string_view manifest_url) { 
     std::string game_id_str(game_id);
     std::string manifest_url_str(manifest_url);
     LOG(INFO) << "Starting InstallGame for " << game_id_str << " from manifest: " << manifest_url_str;

     // --- Pre-Task Setup --- 
 
     // 1. Get install path and create directory
     absl::StatusOr<fs::path> install_path_status = FindInstallPath(game_id_str);
     if (!install_path_status.ok()) {
         LOG(ERROR) << "Failed to get install path for game " << game_id_str << ": " << install_path_status.status();
         return install_path_status.status();
     }
     fs::path install_path = install_path_status.value();

     absl::Status dir_status = EnsureDirectoryExists(install_path);
     if (!dir_status.ok()) {
         LOG(ERROR) << "Failed to create install directory " << install_path.string() << ": " << dir_status;
         return dir_status;
     }

     // --- Define Background Task --- 
     auto installation_task = 
         [this, game_id_str, manifest_url_str, install_path](ProgressReporter progress_reporter) -> bool {
         // RAII cleanup using absl::Cleanup
         auto cleanup = absl::MakeCleanup([this, game_id_str] {
             absl::MutexLock lock(&active_operations_mutex_);
             active_operations_.erase(game_id_str); // Use captured game_id
             LOG(INFO) << "Cleaned up active operation entry for game: " << game_id_str;
         });

         try {
             // 2. Download and Parse Manifest
             progress_reporter(0.05f, "Downloading manifest...");
             absl::StatusOr<std::string> manifest_content = DownloadString(manifest_url_str);
             if (!manifest_content.ok()) {
                 LOG(ERROR) << "Failed to download manifest: " << manifest_content.status();
                 return false;
             }
             LOG(INFO) << "Manifest downloaded successfully.";

             progress_reporter(0.10f, "Parsing manifest...");
             json manifest_data;
             try {
                 manifest_data = json::parse(*manifest_content);
             } catch (const json::parse_error& e) {
                 LOG(ERROR) << "Failed to parse manifest JSON: " << e.what();
                 return false;
             }
             LOG(INFO) << "Manifest parsed successfully.";

             progress_reporter(0.15f, "Validating manifest...");
             // 3. Validate Manifest (Simplified - should be more robust)
             if (!manifest_data.contains("files") || !manifest_data["files"].is_array()) { 
                 LOG(ERROR) << "Manifest validation failed: Missing or invalid 'files' array.";
                 return false;
             }
             // ... (Add more validation as before if needed)
             LOG(INFO) << "Manifest validated successfully for game: " << game_id_str;

             // Start of file processing covers range 15% - 95%
             double base_progress = 0.15;
             double file_progress_range = 0.80; // 15% to 95%

             // 4. Download and Verify files
             const json& files_array = manifest_data["files"];
             const size_t total_files = files_array.empty() ? 1 : files_array.size(); 
             int files_processed = 0; 

             for (const auto& file_entry : files_array) {
                 progress_reporter(static_cast<float>(base_progress + (files_processed / static_cast<double>(total_files) * file_progress_range)), absl::StrCat("Downloading: ", file_entry["path"].get<std::string>()));
                 std::string relative_path_str = file_entry["path"].get<std::string>();
                 std::string file_url = file_entry["url"].get<std::string>();
                 std::string file_hash = file_entry["hash"].get<std::string>();

                 fs::path destination_path = install_path / fs::path(relative_path_str).lexically_normal();

                 // Ensure the directory for the file exists
                  try {
                      if (destination_path.has_parent_path()) {
                          fs::create_directories(destination_path.parent_path());
                      }
                  } catch (const fs::filesystem_error& e) {
                      LOG(ERROR) << "Failed to create directory for file " << relative_path_str << ": " << e.what();
                      return false;
                  }

                 LOG(INFO) << "Processing file [" << (files_processed + 1) << "/" << total_files << "]: " << relative_path_str;

                 // Calculate progress and report
                 double file_fraction = static_cast<double>(files_processed) / total_files;
                 double download_start_progress = base_progress + (file_fraction * file_progress_range);
                 progress_reporter(static_cast<float>(download_start_progress), absl::StrCat("Downloading: ", relative_path_str));

                 LOG(INFO) << "Downloading " << file_url << " to " << destination_path.string();
                 absl::Status download_status = DownloadFile(file_url, destination_path);
                 if (!download_status.ok()) {
                      LOG(ERROR) << "Download failed for file " << relative_path_str << ": " << download_status;
                      return false;
                  }

                 absl::Status hash_status = VerifyFileHash(destination_path, file_hash); // Re-enabled call
                 if (!hash_status.ok()) { // Check status instead
                     progress_reporter((files_processed + 1.0f) / total_files, absl::StrCat("Hash mismatch for: ", destination_path.filename().string()));
                     LOG(ERROR) << "Verification failed for " << destination_path.string() << ": " << hash_status;
                     // Optional: Decide whether to continue other tasks or fail fast
                     return false;
                 }

                 files_processed++;
             }

             progress_reporter(0.95f, "Saving local manifest...");
             // 5. Save local manifest copy
             absl::Status save_status = SaveLocalManifest(game_id_str, install_path, manifest_data);
             if (!save_status.ok()) {
                 LOG(ERROR) << "Failed to save local manifest for game " << game_id_str << ": " << save_status;
                 return false;
             }

             progress_reporter(1.0f, "Installation complete.");
             LOG(INFO) << "Background installation task completed successfully for " << game_id_str;
             return true; // Indicate success

         } catch (const std::exception& e) {
             LOG(ERROR) << "Exception during installation task for game " << game_id_str << ": " << e.what();
             return false;
         } catch (...) {
             LOG(ERROR) << "Unknown exception during installation task for game " << game_id_str;
             return false;
         }
     };

     // --- Start Background Task --- 
     std::string task_description = absl::StrCat("Installing ", game_id_str);
     TaskId task_id = background_task_manager_->StartTask(installation_task, task_description);
     LOG(INFO) << "Installation task started for game " << game_id_str << " with Task ID: " << task_id;

     // --- Register Active Operation --- 
     {
         absl::MutexLock lock(&active_operations_mutex_);
         // Check if another operation is already running for this game
         if (active_operations_.count(game_id_str)) {
              LOG(ERROR) << "Installation requested for game '" << game_id_str 
                        << "', but another operation is already in progress.";
              // Attempt to cancel the newly created task as it shouldn't run
              background_task_manager_->RequestCancellation(task_id);
              return absl::AlreadyExistsError(absl::StrCat("Operation already in progress for game: ", game_id_str));
         }
         active_operations_[game_id_str] = task_id;
     }

     // InstallGame now returns OkStatus immediately after starting the background task.
     // The actual success/failure is handled by the background task status.
     return absl::OkStatus(); 
 }

absl::Status BasicGameManagementService::UpdateGame(std::string_view game_id) {
    std::string game_id_str(game_id);
    LOG(INFO) << "Starting UpdateGame for " << game_id_str;

    // --- Pre-Task Setup --- 
    // 1. Get install path and create directory
     absl::StatusOr<fs::path> install_path_status = FindInstallPath(game_id_str);
     if (!install_path_status.ok()) {
         LOG(ERROR) << "Failed to get install path for game " << game_id_str << ": " << install_path_status.status();
         return install_path_status.status();
     }
     fs::path install_path = install_path_status.value();

     // 2. Get manifest URL
     std::string manifest_url = "https://example.com/manifests/" + game_id_str + "/latest.json"; // Placeholder

     // --- Define Background Task --- 
     auto update_task = 
         [this, game_id_str, install_path, manifest_url](ProgressReporter progress_reporter) -> bool {
         // RAII cleanup using absl::Cleanup
         auto cleanup = absl::MakeCleanup([this, game_id_str] {
             absl::MutexLock lock(&active_operations_mutex_);
             active_operations_.erase(game_id_str); // Use captured game_id
             LOG(INFO) << "Cleaned up active operation entry for game: " << game_id_str;
         });

         try {
             // 2. Download and Parse Manifest
             progress_reporter(0.05f, "Downloading manifest...");
             absl::StatusOr<std::string> manifest_content = DownloadString(manifest_url);
             if (!manifest_content.ok()) {
                 LOG(ERROR) << "Failed to download manifest: " << manifest_content.status();
                 return false;
             }
             LOG(INFO) << "Manifest downloaded successfully.";

             progress_reporter(0.10f, "Parsing manifest...");
             json manifest_data;
             try {
                 manifest_data = json::parse(*manifest_content);
             } catch (const json::parse_error& e) {
                 LOG(ERROR) << "Failed to parse manifest JSON: " << e.what();
                 return false;
             }
             LOG(INFO) << "Manifest parsed successfully.";

             progress_reporter(0.15f, "Validating manifest...");
             // 3. Validate Manifest (Simplified - should be more robust)
             if (!manifest_data.contains("files") || !manifest_data["files"].is_array()) { 
                 LOG(ERROR) << "Manifest validation failed: Missing or invalid 'files' array.";
                 return false;
             }
             // ... (Add more validation as before if needed)
             LOG(INFO) << "Manifest validated successfully for game: " << game_id_str;

             // Start of file processing covers range 15% - 95%
             double base_progress = 0.15;
             double file_progress_range = 0.80; // 15% to 95%

             // 4. Download and Verify files
             const json& files_array = manifest_data["files"];
             const size_t total_files = files_array.empty() ? 1 : files_array.size(); 
             int files_processed = 0; 

             for (const auto& file_entry : files_array) {
                 progress_reporter(static_cast<float>(base_progress + (files_processed / static_cast<double>(total_files) * file_progress_range)), absl::StrCat("Downloading: ", file_entry["path"].get<std::string>()));
                 std::string relative_path_str = file_entry["path"].get<std::string>();
                 std::string file_url = file_entry["url"].get<std::string>();
                 std::string file_hash = file_entry["hash"].get<std::string>();

                 fs::path destination_path = install_path / fs::path(relative_path_str).lexically_normal();

                 // Ensure the directory for the file exists
                  try {
                      if (destination_path.has_parent_path()) {
                          fs::create_directories(destination_path.parent_path());
                      }
                  } catch (const fs::filesystem_error& e) {
                      LOG(ERROR) << "Failed to create directory for file " << relative_path_str << ": " << e.what();
                      return false;
                  }

                 LOG(INFO) << "Processing file [" << (files_processed + 1) << "/" << total_files << "]: " << relative_path_str;

                 // Calculate progress and report
                 double file_fraction = static_cast<double>(files_processed) / total_files;
                 double download_start_progress = base_progress + (file_fraction * file_progress_range);
                 progress_reporter(static_cast<float>(download_start_progress), absl::StrCat("Downloading: ", relative_path_str));

                 LOG(INFO) << "Downloading " << file_url << " to " << destination_path.string();
                 absl::Status download_status = DownloadFile(file_url, destination_path);
                 if (!download_status.ok()) {
                      LOG(ERROR) << "Download failed for file " << relative_path_str << ": " << download_status;
                      return false;
                  }

                 absl::Status hash_status = VerifyFileHash(destination_path, file_hash); // Re-enabled call
                 if (!hash_status.ok()) { // Check status instead
                     progress_reporter((files_processed + 1.0f) / total_files, absl::StrCat("Hash mismatch for: ", destination_path.filename().string()));
                     LOG(ERROR) << "Verification failed for " << destination_path.string() << ": " << hash_status;
                     // Optional: Decide whether to continue other tasks or fail fast
                     return false;
                 }

                 files_processed++;
             }

             progress_reporter(0.95f, "Saving local manifest...");
             // 5. Save local manifest copy
             absl::Status save_status = SaveLocalManifest(game_id_str, install_path, manifest_data);
             if (!save_status.ok()) {
                 LOG(ERROR) << "Failed to save local manifest for game " << game_id_str << ": " << save_status;
                 return false;
             }

             progress_reporter(1.0f, "Update complete.");
             LOG(INFO) << "Background update task completed successfully for " << game_id_str;
             return true; // Indicate success

         } catch (const std::exception& e) {
             LOG(ERROR) << "Exception during update task for game " << game_id_str << ": " << e.what();
             return false;
         } catch (...) {
             LOG(ERROR) << "Unknown exception during update task for game " << game_id_str;
             return false;
         }
     };

     // --- Start Background Task --- 
     std::string task_description = absl::StrCat("Updating ", game_id_str);
     TaskId task_id = background_task_manager_->StartTask(update_task, task_description);
     LOG(INFO) << "Update task started for game " << game_id_str << " with Task ID: " << task_id;

     // --- Register Active Operation --- 
     {
         absl::MutexLock lock(&active_operations_mutex_);
         // Check if another operation is already running for this game
         if (active_operations_.count(game_id_str)) {
              LOG(ERROR) << "Update requested for game '" << game_id_str 
                        << "', but another operation is already in progress.";
              background_task_manager_->RequestCancellation(task_id);
              return absl::AlreadyExistsError(absl::StrCat("Operation already in progress for game: ", game_id_str));
         }
         active_operations_[game_id_str] = task_id;
     }

     // InstallGame now returns OkStatus immediately after starting the background task.
     // The actual success/failure is handled by the background task status.
     return absl::OkStatus(); 
 }

absl::Status BasicGameManagementService::VerifyGame(std::string_view game_id_sv) {
    std::string game_id(game_id_sv);
    LOG(INFO) << "Starting VerifyGame for " << game_id;

    // --- Pre-Task Setup --- 
    // 1. Get install path and create directory
     absl::StatusOr<fs::path> install_path_status = FindInstallPath(game_id);
     if (!install_path_status.ok()) {
         LOG(ERROR) << "Failed to get install path for game " << game_id << ": " << install_path_status.status();
         return install_path_status.status();
     }
     fs::path install_path = install_path_status.value();

     // --- Define Background Task --- 
     auto verification_task = 
         [this, game_id, install_path](ProgressReporter progress_reporter) -> bool {
         // RAII cleanup using absl::Cleanup
         auto cleanup = absl::MakeCleanup([this, game_id] {
             absl::MutexLock lock(&active_operations_mutex_);
             active_operations_.erase(game_id);
             LOG(INFO) << "Cleaned up active operation entry for game: " << game_id;
         });

         try {
             // 2. Load Local Manifest
             progress_reporter(0.05f, "Loading local manifest...");
             absl::StatusOr<json> local_manifest_status = LoadLocalManifest(game_id, install_path);
             if (!local_manifest_status.ok()) {
                 LOG(ERROR) << "Failed to load local manifest for game " << game_id << ": " << local_manifest_status.status();
                 return false;
             }
             json local_manifest = local_manifest_status.value();

             // 3. Check Manifest Structure (Basic)
             if (!local_manifest.contains("files")) {
                 LOG(ERROR) << "Local manifest for game " << game_id << " is missing 'files' field.";
                 return false;
             }

             const json& files_to_verify = local_manifest["files"];
             size_t total_files = files_to_verify.size();
             size_t files_processed = 0;
             bool verification_failed = false;
             std::vector<std::string> failed_files;

             if (total_files == 0) {
                 LOG(WARNING) << "Local manifest for game " << game_id << " contains no files to verify.";
             } else {
                  // 4. Iterate and Verify Files (Placeholder)
                 for (auto it = files_to_verify.begin(); it != files_to_verify.end(); ++it) {
                     std::string relative_path_str = it.key();
                     fs::path full_path = install_path / relative_path_str;

                     double current_progress = 0.1 + (0.85 * files_processed / total_files); // Progress range 10% - 95%
                     progress_reporter(static_cast<float>(current_progress), absl::StrCat("Verifying ", relative_path_str, "..."));
                     // Placeholder verification logic
                     // TODO: Implement actual hash checking (e.g., using SHA256)
                     bool file_exists = fs::exists(full_path);
                     bool hash_matches = true; // Assume true for placeholder
                     std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate work
                     
                     // Example: Replace with actual check
                     // std::string expected_hash = it.value().value("hash", "");
                     // if (!expected_hash.empty()) { hash_matches = CalculateFileHash(full_path) == expected_hash; }
                     
                     if (!file_exists || !hash_matches) {
                         LOG(WARNING) << "Verification failed for file: " << relative_path_str 
                                      << " (Exists: " << file_exists << ", Hash Match: " << hash_matches << ")";
                         verification_failed = true;
                         failed_files.push_back(relative_path_str);
                         // Optionally, break early? Or report all failed files?
                         // For now, continue checking all files.
                     }
                     files_processed++;
                 }
             }

             // 5. Finalize Status
             if (verification_failed) {
                  std::string error_msg = "Verification failed for files: ";
                  for(size_t i = 0; i < failed_files.size(); ++i) {
                     error_msg += failed_files[i] + (i == failed_files.size() - 1 ? "" : ", ");
                  }
                  LOG(ERROR) << error_msg;
                  progress_reporter(1.0f, "Verification failed.");
                  return false; // Indicate failure
             } else {
                 progress_reporter(1.0f, "Verification complete.");
                 LOG(INFO) << "Verification task completed successfully for game: " << game_id;
                 return true; // Indicate success
             }
         } catch (const std::exception& e) {
             LOG(ERROR) << "Exception during verification task for game " << game_id << ": " << e.what();
             return false;
         }
     };

     // --- Start Background Task --- 
     std::string task_description = absl::StrCat("Verifying ", game_id);
     TaskId task_id = background_task_manager_->StartTask(verification_task, task_description);
     LOG(INFO) << "Verification task started for game " << game_id << " with Task ID: " << task_id;

     // --- Register Active Operation --- 
     {
         absl::MutexLock lock(&active_operations_mutex_);
         // Check if another operation is already running for this game
         if (active_operations_.count(game_id)) {
              LOG(ERROR) << "Verification requested for game '" << game_id 
                        << "', but another operation is already in progress.";
              background_task_manager_->RequestCancellation(task_id);
              return absl::AlreadyExistsError(absl::StrCat("Operation already in progress for game: ", game_id));
         }
         active_operations_[game_id] = task_id;
     }

     return absl::OkStatus(); 
}

absl::StatusOr<fs::path> BasicGameManagementService::FindInstallPath(std::string_view game_id_sv) {
    // For now, this uses a simple convention. Could be extended to read a config file.
    std::string game_id(game_id_sv);
    fs::path base_install_dir = GetBaseInstallDirectory(); // Assumes this helper exists
    fs::path install_path = base_install_dir / game_id;

    // Check if the directory actually exists (it might if the game is installed)
    // This function might just *determine* the path, not guarantee existence.
    // Let's assume it just returns the expected path for now.
    return install_path;
}

fs::path BasicGameManagementService::GetBaseInstallDirectory() {
    // TODO: Make this configurable (e.g., read from user settings)
    // For now, hardcode a path relative to the executable or user documents
    // Using current path + "Games" for simplicity during development.
    return fs::current_path() / "Games";
}

absl::Status BasicGameManagementService::EnsureDirectoryExists(const fs::path& dir_path) {
     if (!fs::exists(dir_path)) {
         LOG(INFO) << "Creating directory: " << dir_path.string();
         std::error_code ec;
         if (!fs::create_directories(dir_path, ec)) {
             // Check if the error was simply because the directory already exists (race condition?)
             if (!fs::exists(dir_path)) { // Re-check existence after create attempt
                 LOG(ERROR) << "Failed to create directory " << dir_path.string() << ": " << ec.message();
                 return absl::InternalError(absl::StrCat("Failed to create directory: ", ec.message()));
             }
         }
     }
     return absl::OkStatus();
}

absl::Status BasicGameManagementService::SaveLocalManifest(std::string_view game_id_sv, const fs::path& install_path, const json& manifest_data) {
    std::string game_id(game_id_sv);
    fs::path manifest_dir = install_path / ".launcher_metadata";
    fs::path manifest_path = manifest_dir / (game_id + "_manifest.json");
 
    absl::Status dir_status = EnsureDirectoryExists(manifest_dir);
    if (!dir_status.ok()) {
        LOG(ERROR) << "Failed to ensure manifest directory exists: " << manifest_dir.string() << ": " << dir_status;
        return dir_status;
    }
 
    try {
        std::ofstream file_stream(manifest_path);
        if (!file_stream.is_open()) {
            LOG(ERROR) << "Failed to open local manifest file for writing: " << manifest_path.string();
            return absl::InternalError(absl::StrCat("Failed to open local manifest file for writing: ", manifest_path.string()));
        }
        file_stream << manifest_data.dump(4); // Pretty print with 4 spaces
        file_stream.close();
        LOG(INFO) << "Successfully saved local manifest for game " << game_id << " to " << manifest_path.string();
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception saving local manifest for game " << game_id << ": " << e.what();
        return absl::InternalError(absl::StrCat("Exception saving local manifest: ", e.what()));
    }
 
    return absl::OkStatus();
}

absl::StatusOr<json> BasicGameManagementService::LoadLocalManifest(std::string_view game_id_sv, const fs::path& install_path) const { // Added const
    std::string game_id(game_id_sv);
    fs::path manifest_path = install_path / ".launcher_metadata" / (game_id + "_manifest.json");
 
    if (!fs::exists(manifest_path)) {
        LOG(INFO) << "Local manifest not found for game " << game_id << " at " << manifest_path.string();
        return absl::NotFoundError("Manifest file not found.");
    }
 
    std::ifstream file_stream(manifest_path);
    if (!file_stream.is_open()) {
        LOG(ERROR) << "Failed to open local manifest file for reading: " << manifest_path.string();
        return absl::InternalError(absl::StrCat("Failed to open local manifest file for reading: ", manifest_path.string()));
    }
 
    try {
        json data = json::parse(file_stream); // Parse directly from stream
        file_stream.close();
        LOG(INFO) << "Successfully loaded and parsed local manifest for game: " << game_id;
        return data;
    } catch (const json::parse_error& e) {
        LOG(ERROR) << "Failed to parse local manifest JSON for game " << game_id << " at " << manifest_path.string() << ": " << e.what();
        return absl::InvalidArgumentError(absl::StrCat("Invalid JSON in local manifest: ", e.what()));
    } catch (const std::exception& e) {
        LOG(ERROR) << "Exception loading local manifest for game " << game_id << ": " << e.what();
        return absl::InternalError(absl::StrCat("Exception loading local manifest: ", e.what()));
    }
}

absl::Status BasicGameManagementService::VerifyFileHash(const fs::path& file_path, const std::string& expected_hash) {
    LOG(INFO) << "Verifying hash for " << file_path.string(); //<< " against expected hash " << expected_hash;
 
    if (expected_hash.empty()) {
        LOG(WARNING) << "Skipping hash verification for " << file_path.string() << " due to empty expected hash.";
        return absl::OkStatus(); // Or perhaps an InvalidArgumentError?
    }
 
    std::ifstream file_stream(file_path, std::ios::binary);
    if (!file_stream) {
        LOG(ERROR) << "Failed to open file for hashing: " << file_path.string();
        return absl::NotFoundError(absl::StrCat("Failed to open file for hashing: ", file_path.string()));
    }
 
    // Calculate the hash using picosha2
    std::string actual_hash;
    try {
         actual_hash = picosha2::hash256_hex_string(std::istreambuf_iterator<char>(file_stream),
                                                std::istreambuf_iterator<char>());
    } catch (const std::exception& e) {
         LOG(ERROR) << "Exception during hash calculation for " << file_path.string() << ": " << e.what();
         file_stream.close();
         return absl::InternalError(absl::StrCat("Hashing failed: ", e.what()));
    } catch (...) {
         LOG(ERROR) << "Unknown exception during hash calculation for " << file_path.string();
         file_stream.close();
         return absl::InternalError("Unknown hashing error.");
    }
    
    file_stream.close(); // Close the file stream explicitly after hashing
 
    // Compare hashes (case-insensitive comparison is safer for hex strings)
    if (absl::EqualsIgnoreCase(actual_hash, expected_hash)) {
        LOG(INFO) << "Hash verification successful for: " << file_path.string();
        return absl::OkStatus();
    } else {
        LOG(ERROR) << "Hash mismatch for file: " << file_path.string() 
                   << ". Expected: " << expected_hash << ", Actual: " << actual_hash;
        return absl::DataLossError(absl::StrCat("Hash mismatch for \"", file_path.filename().string(), "\""));
    }
}

// Factory function implementation
std::shared_ptr<IGameManagementService> CreateBasicGameManager(
    const std::filesystem::path& games_json_path,
    std::shared_ptr<IUserSettings> user_settings,
    IBackgroundTaskManager* background_task_manager) {

    // Optional: Add null check for background_task_manager if required
    // if (!background_task_manager) { ... handle error ... }

    LOG(INFO) << "Creating BasicGameManagementService instance with JSON path: " << games_json_path.string();
    return std::make_shared<BasicGameManagementService>(
        games_json_path,
        std::move(user_settings), // Move user_settings if appropriate
        background_task_manager);
}

} // namespace core
} // namespace game_launcher
