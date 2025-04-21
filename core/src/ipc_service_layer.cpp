// core/src/ipc_service_layer.cpp
#include "game_launcher/core/ipc_service_layer.h"

#include <iostream> // For logging
#include <unordered_map>
#include <string>
#include <mutex>    // For std::mutex, std::lock_guard
#include <optional> // Included via header, but good practice

namespace game_launcher {
namespace core {

// Basic placeholder implementation of IIPCServiceLayer.
// Doesn't actually perform IPC, just logs and manages handlers.
class BasicIPCServiceLayer : public IIPCServiceLayer {
 public:
  BasicIPCServiceLayer() = default;
  ~BasicIPCServiceLayer() override = default;

  // Non-copyable/movable
  BasicIPCServiceLayer(const BasicIPCServiceLayer&) = delete;
  BasicIPCServiceLayer& operator=(const BasicIPCServiceLayer&) = delete;
  BasicIPCServiceLayer(BasicIPCServiceLayer&&) = delete;
  BasicIPCServiceLayer& operator=(BasicIPCServiceLayer&&) = delete;

  bool SendEventToUI(std::string_view event_name, std::string_view payload) override {
    // In a real implementation (likely in cef_integration):
    // - Serialize payload if necessary (e.g., ensure valid JSON).
    // - Find the target browser/frame in CEF.
    // - Construct a JavaScript call (e.g., `window.dispatchEvent(new CustomEvent('${event_name}', { detail: ${payload} }));`).
    // - Execute the JavaScript via CEF's ExecuteJavaScript method.
    // - Handle potential errors during execution.
    std::cout << "BasicIPCServiceLayer: [EVENT -> UI] Name: " << event_name
              << ", Payload: " << payload << std::endl;
    return true; // Assume success for now
  }

  bool RegisterUIRequestHandler(std::string_view message_name, UIRequestHandler handler) override {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    std::string name_str(message_name);
    if (handlers_.count(name_str)) {
      std::cerr << "BasicIPCServiceLayer: Handler already registered for '" << message_name << "'" << std::endl;
      return false; // Already registered
    }
    handlers_[name_str] = std::move(handler);
    std::cout << "BasicIPCServiceLayer: Registered handler for '" << message_name << "'" << std::endl;
    return true;
  }

  bool UnregisterUIRequestHandler(std::string_view message_name) override {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    std::string name_str(message_name);
    auto it = handlers_.find(name_str);
    if (it == handlers_.end()) {
      std::cerr << "BasicIPCServiceLayer: No handler found for '" << message_name << "' to unregister." << std::endl;
      return false; // Not found
    }
    handlers_.erase(it);
    std::cout << "BasicIPCServiceLayer: Unregistered handler for '" << message_name << "'" << std::endl;
    return true;
  }

  // Method to simulate receiving a request from the UI (for testing/later use)
  std::optional<std::string> SimulateRequestFromUI(std::string_view message_name, std::string_view payload) {
       std::cout << "BasicIPCServiceLayer: [REQUEST <- UI] Name: " << message_name
                 << ", Payload: " << payload << std::endl;
       UIRequestHandler handler;
       {
            std::lock_guard<std::mutex> lock(handlers_mutex_);
            std::string name_str(message_name);
            auto it = handlers_.find(name_str);
            if (it == handlers_.end()) {
                std::cerr << "BasicIPCServiceLayer: No handler registered for received message '" << message_name << "'" << std::endl;
                return std::nullopt; // Or return an error payload
            }
            handler = it->second; // Copy the function object
       }

        // Execute the handler outside the lock
        try {
            // Note: The handler might interact with other core modules.
            return handler(message_name, payload);
        } catch (const std::exception& e) {
            std::cerr << "BasicIPCServiceLayer: Exception caught while executing handler for '"
                      << message_name << "': " << e.what() << std::endl;
            return std::nullopt; // Indicate error
        } catch (...) {
             std::cerr << "BasicIPCServiceLayer: Unknown exception caught while executing handler for '"
                       << message_name << "'" << std::endl;
             return std::nullopt; // Indicate error
        }
  }


 private:
  mutable std::mutex handlers_mutex_; // Protects access to the handlers map
  std::unordered_map<std::string, UIRequestHandler> handlers_;
};

// TODO: Add a factory function or mechanism to create instances

} // namespace core
} // namespace game_launcher
