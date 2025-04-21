// core/include/game_launcher/core/ipc_service_layer.h
#ifndef GAME_LAUNCHER_CORE_IPC_SERVICE_LAYER_H_
#define GAME_LAUNCHER_CORE_IPC_SERVICE_LAYER_H_

#include <functional>
#include <string>
#include <string_view>
#include <optional> // For potential request/response patterns

// Forward declarations (if needed for dependencies passed to handlers)
// namespace game_launcher { namespace core { class ...; } }

namespace game_launcher {
namespace core {

// Type definition for a handler function that processes messages from the UI.
// Takes message name and payload (e.g., JSON string) as input.
// Returns an optional string payload as a response (for request/response).
using UIRequestHandler = std::function<std::optional<std::string>(
    std::string_view /* message_name */,
    std::string_view /* payload */
)>;

// Interface for the Inter-Process Communication (IPC) Service Layer.
// Connects the C++ backend with the UI frontend (e.g., CEF JavaScript).
class IIPCServiceLayer {
 public:
  virtual ~IIPCServiceLayer() = default;

  // Sends an event or message from the backend to the UI.
  // event_name: Identifier for the event (e.g., "loginStatusChanged", "downloadProgress").
  // payload: Data associated with the event (often serialized, e.g., JSON string).
  // Returns true if the message was successfully queued or sent, false otherwise.
  virtual bool SendEventToUI(std::string_view event_name, std::string_view payload) = 0;

  // Registers a handler function to be called when a specific message/request
  // is received from the UI.
  // message_name: The identifier the UI will use to send this request.
  // handler: The function to execute when the message is received.
  // Returns true if the handler was registered successfully, false if the name is already taken.
  virtual bool RegisterUIRequestHandler(std::string_view message_name, UIRequestHandler handler) = 0;

  // Unregisters a previously registered handler.
  // Returns true if a handler was found and removed, false otherwise.
  virtual bool UnregisterUIRequestHandler(std::string_view message_name) = 0;

  // TODO: Consider adding methods for initializing/shutting down the IPC connection.
  // TODO: Define specific payload formats (e.g., enforce JSON).
  // TODO: Add more robust error handling and reporting.
};

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_IPC_SERVICE_LAYER_H_
