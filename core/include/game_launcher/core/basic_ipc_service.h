// core/include/game_launcher/core/basic_ipc_service.h
#ifndef GAME_LAUNCHER_CORE_BASIC_IPC_SERVICE_H_
#define GAME_LAUNCHER_CORE_BASIC_IPC_SERVICE_H_

#include "game_launcher/core/ipc_service.h"

#include <map> // Keep standard map include for potential future reference or comparison
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
// #include "absl/log/log.h" // REMOVED: Core module should not directly use Abseil logging.
#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"

namespace game_launcher {
namespace core {

// Basic implementation of the IPC service.
// This class is responsible for managing request handlers and dispatching
// incoming requests to the appropriate handler.
// It assumes the underlying transport mechanism (e.g., CEF message router)
// will call HandleIncomingRequest.
class BasicIPCService : public IIPCService {
public:
    BasicIPCService();
    ~BasicIPCService() override = default;

    // --- IIPCService Implementation ---

    // Registers a handler for a specific request type. Thread-safe.
    absl::Status RegisterRequestHandler(std::string_view request_name, RequestHandler handler) override;

    // Retrieves the application version string. Thread-safe (as it returns a copy).
    absl::StatusOr<std::string> GetVersion() override;

    // Attempts to launch the specified game. Thread-safe (assumes launch logic is).
    absl::Status LaunchGame(std::string_view game_id) override;

    // --- BasicIPCService Specific Methods ---

    // Method to be called by the underlying transport layer (e.g., CEF)
    // when a request is received from the frontend. Thread-safe.
    // Args:
    //   request_name: The identifier of the request type.
    //   request_payload: The data payload associated with the request.
    //   response_callback: The callback to send the response back to the frontend.
    void HandleIncomingRequest(std::string_view request_name,
                               const IpcPayload& request_payload,
                               ResponseCallback response_callback);

private:
    absl::Mutex handlers_mutex_;
    // Using flat_hash_map for potentially better performance than std::map
    absl::flat_hash_map<std::string, RequestHandler> request_handlers_ ABSL_GUARDED_BY(handlers_mutex_);
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_BASIC_IPC_SERVICE_H_
