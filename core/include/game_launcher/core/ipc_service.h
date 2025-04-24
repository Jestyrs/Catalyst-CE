// core/include/game_launcher/core/ipc_service.h
#ifndef GAME_LAUNCHER_CORE_IPC_SERVICE_H_
#define GAME_LAUNCHER_CORE_IPC_SERVICE_H_

#include <functional>
#include <string>
#include <string_view>
#include <vector>        // For returning game statuses
#include <memory>
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "game_launcher/core/game_status.h"         // Include game state definitions
#include "game_launcher/core/game_status_listener.h" // Include listener interface
#include "game_launcher/core/auth_status.h"         // Include auth status definitions
#include "game_launcher/core/user_profile.h"        // Include user profile definitions
#include "game_launcher/core/app_settings.h"        // Include AppSettings definition

namespace game_launcher {
namespace core {

// Interface for the core service responsible for game management,
// installation, launching, and communicating status changes via listeners.
class IIPCService {
public:
    virtual ~IIPCService() = default;

    // --- Listener Management ---

    // Registers a listener to receive game status updates.
    // The listener pointer must remain valid for the lifetime of the subscription.
    // The caller is responsible for managing the listener's lifetime.
    // It is recommended to call RemoveStatusListener before the listener is destroyed.
    // This method should be thread-safe if listeners can be added/removed concurrently.
    virtual void AddStatusListener(IGameStatusListener* listener) = 0;

    // Unregisters a previously registered listener.
    // No-op if the listener was not registered.
    // This method should be thread-safe.
    virtual void RemoveStatusListener(IGameStatusListener* listener) = 0;

    // --- Synchronous Requests ---

    // Retrieves the application version string.
    virtual absl::StatusOr<std::string> GetVersion() = 0;

    // Retrieves the initial status of all known/managed games.
    // This is typically called once by the UI upon initialization.
    // Subsequent updates are pushed via the IGameStatusListener interface.
    virtual absl::StatusOr<std::vector<GameStatusUpdate>> GetInitialGameStatuses() = 0;

    // --- Asynchronous Action Requests ---
    // These methods initiate potentially long-running operations.
    // Success/failure/progress is reported asynchronously via IGameStatusListener.
    // The return status indicates if the request was *accepted* for processing,
    // not the final result of the operation.

    // Requests the installation of the specified game.
    virtual absl::Status RequestInstall(std::string_view game_id) = 0;

    // Requests the launch of the specified game.
    virtual absl::Status RequestLaunch(std::string_view game_id) = 0;

    // Requests an update check and potential update for the specified game.
    virtual absl::Status RequestUpdate(std::string_view game_id) = 0;

    // Requests cancellation of any ongoing operation (install/update) for the specified game.
    virtual absl::Status RequestCancel(std::string_view game_id) = 0;

    // --- Authentication ---

    /**
     * @brief Attempts to log in a user.
     * @param username_sv User's username.
     * @param password_sv User's password.
     * @return OK status on success, UnauthenticatedError on failure.
     */
    virtual absl::Status Login(std::string_view username_sv, std::string_view password_sv) = 0;

    /**
     * @brief Logs out the current user.
     * @return OK status.
     */
    virtual absl::Status Logout() = 0;

    /**
     * @brief Gets the current authentication status.
     * @return The current AuthStatus.
     */
    virtual AuthStatus GetAuthStatus() const = 0;

    /**
     * @brief Gets the profile of the currently logged-in user.
     * @return StatusOr containing the UserProfile if logged in, NotFoundError otherwise.
     */
    virtual absl::StatusOr<UserProfile> GetCurrentUserProfile() const = 0;

    // --- Application Settings ---

    virtual AppSettings GetAppSettings() = 0;
    virtual absl::Status SetAppSettings(const AppSettings& settings) = 0;

    // TODO: Add methods for updating, verifying, uninstalling games etc. as needed.

    // TODO: Re-evaluate if the old RequestHandler mechanism is still needed or should be removed/adapted.
    /*
    // Type alias for the payload of an IPC request or response.
    using IpcPayload = std::string;
    // Callback function type for sending a response back to the frontend.
    using ResponseCallback = std::function<void(absl::StatusOr<IpcPayload>)>;
    // Function type for handling an incoming request from the frontend.
    using RequestHandler = std::function<void(const IpcPayload& request_payload, ResponseCallback response_callback)>;
    // Registers a handler for a specific request type originating from the frontend.
    virtual absl::Status RegisterRequestHandler(std::string_view request_name, RequestHandler handler) = 0;
    */
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_IPC_SERVICE_H_
