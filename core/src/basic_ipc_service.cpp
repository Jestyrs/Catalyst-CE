// core/src/basic_ipc_service.cpp
#include "game_launcher/core/basic_ipc_service.h"

// #include "include/base/cef_logging.h" // REMOVED: Core module shouldn't use CEF logging directly.
#include "absl/strings/str_cat.h"

namespace game_launcher {
namespace core {

BasicIPCService::BasicIPCService() {
    // LOG(INFO) << "BasicIPCService initialized."; // REMOVED
}

// absl::Status BasicIPCService::RegisterRequestHandler(std::string_view request_name, RequestHandler handler) {
//     absl::MutexLock lock(&handlers_mutex_);
//     std::string name(request_name); // Convert to string for map key
//     if (request_handlers_.contains(name)) {
//         // LOG(ERROR) << "Attempted to register duplicate request handler: " << name; // REMOVED
//         return absl::AlreadyExistsError(absl::StrCat("Handler already registered for request: ", name));
//     }
//     request_handlers_[name] = std::move(handler);
//     // LOG(INFO) << "Registered request handler for: " << name; // REMOVED
//     return absl::OkStatus();
// }

absl::StatusOr<std::string> BasicIPCService::GetVersion() {
    // LOG(INFO) << "BasicIPCService::GetVersion() called."; // REMOVED
    // TODO: Replace with actual version retrieval (e.g., from build info or config)
    return std::string("0.1.0"); 
}

// absl::Status BasicIPCService::LaunchGame(std::string_view game_id) {
//     // LOG(INFO) << "BasicIPCService::LaunchGame() called for game_id: " << game_id; // REMOVED
//     // TODO: Implement actual game launching logic, likely by calling GameManagementService.
//     // This might involve finding the game path, creating a process, etc.
//     // Example (needs GameManagementService integration):
//     // return game_management_service_->LaunchGame(game_id);
// 
//     // For now, just indicate the request was received.
//     return absl::OkStatus(); 
// }

// void BasicIPCService::HandleIncomingRequest(std::string_view request_name,
//                                         const IpcPayload& request_payload,
//                                         ResponseCallback response_callback) {
//     RequestHandler handler_to_call;
//     {
//         absl::MutexLock lock(&handlers_mutex_);
//         std::string name(request_name); // Convert to string for map key
//         auto it = request_handlers_.find(name);
//         if (it == request_handlers_.end()) {
//             // LOG(ERROR) << "No handler found for incoming request: " << name; // REMOVED
//             // Call the callback immediately with an error if no handler is found
//             if (response_callback) {
//                  response_callback(absl::NotFoundError(absl::StrCat("No handler registered for request: ", name)));
//             }
//             return; // Exit early
//         }
//         // Copy the handler to call it outside the lock if needed,
//         // although in this simple case it might not be strictly necessary.
//         // It's generally safer if the handler might take time or interact
//         // with other locked resources.
//         handler_to_call = it->second;
//     } // Mutex automatically unlocked here
// 
//     // Call the handler outside the mutex lock
//     if (handler_to_call) {
//          // Use LOG(INFO) for now as VLOG header wasn't found
//          // LOG(INFO) << "Dispatching request: " << request_name; // REMOVED
//         // It's the handler's responsibility to eventually call the response_callback
//         handler_to_call(request_payload, response_callback);
//     } else {
//         // This case should ideally not happen if the map contains valid handlers,
//         // but handle defensively.
//         // LOG(ERROR) << "Found null handler for request: " << request_name; // REMOVED
//          if (response_callback) {
//               response_callback(absl::InternalError(absl::StrCat("Internal error: Null handler found for request: ", request_name)));
//          }
//     }
// } 
 
} // namespace core
} // namespace game_launcher
