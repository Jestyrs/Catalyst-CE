#pragma once

#include <string>
#include <optional> // For optional progress/message
#include <vector>   // For initial state list
#include "absl/status/status.h" // For absl::Status
#include "absl/strings/string_view.h" // For absl::string_view

namespace game_launcher::core {

// Represents the possible states of a game managed by the launcher
// These names will be converted to strings for JavaScript communication.
enum class GameState {
    Unknown,            // Initial state or error retrieving status
    NotInstalled,       // Game is known but not installed
    CheckingStatus,     // Currently verifying installation status/files
    UpdateAvailable,    // Installed, but an update is available (optional feature)
    ReadyToLaunch,      // Installed and ready
    InstallPending,     // Install request received, queued or preparing
    Downloading,        // Currently downloading game files
    Verifying,          // Verifying downloaded files
    Installing,         // Installing game files after download/verification
    LaunchPending,      // Launch request received, preparing to launch
    Launching,          // Process is being created
    Running,            // Game process is confirmed to be running
    InstallFailed,      // Installation process failed
    LaunchFailed,       // Launch attempt failed
    UpdateFailed,       // Update attempt failed (optional feature)
    Idle                // Installed (or partially) but no operation active (e.g., after cancel)
};

// Helper function to convert GameState enum to string (for JS consistency)
// Note: We can align these strings with the ones expected by window.updateGameStatus
inline std::string GameStateToString(GameState state) {
    switch (state) {
        case GameState::Unknown:         return "unknown";
        case GameState::NotInstalled:    return "not_installed";
        case GameState::CheckingStatus:  return "checking_status"; // JS needs handler for this
        case GameState::UpdateAvailable: return "update_available"; // JS needs handler for this
        case GameState::ReadyToLaunch:   return "installed"; // Use 'installed' for JS compatibility
        case GameState::InstallPending:  return "install_pending"; // JS needs handler for this
        case GameState::Downloading:     return "downloading"; // JS needs handler for this
        case GameState::Verifying:       return "verifying"; // JS needs handler for this
        case GameState::Installing:      return "installing"; // JS needs handler for this
        case GameState::LaunchPending:   return "launch_pending"; // JS needs handler for this
        case GameState::Launching:       return "launching"; // JS needs handler for this
        case GameState::Running:         return "running";
        case GameState::InstallFailed:   return "install_failed";
        case GameState::LaunchFailed:    return "launch_failed"; // Might map to 'installed' in JS?
        case GameState::UpdateFailed:    return "update_failed"; // JS needs handler for this
        case GameState::Idle:            return "idle"; // Representing the cancelled/inactive state
        default:                         return "unknown";
    }
}

// Structure to hold status update information pushed from C++ to listeners
struct GameStatusUpdate {
    std::string game_id;
    GameState current_state;
    std::optional<int> progress_percent; // e.g., for download/install
    std::optional<std::string> message;  // Optional additional info/error message
    std::optional<float> bytes_per_second; // Added for download speed
    // Could add more fields like download speed, eta, etc. if needed
};

} // namespace game_launcher::core
