// cef_integration/include/game_launcher/cef_integration/launcher_app.h
#ifndef GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_APP_H_
#define GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_APP_H_

#include "cef_app.h" // Corrected CEF include path
#include "cef_browser_process_handler.h" // Corrected CEF include path
#include "wrapper/cef_message_router.h" // Corrected CEF include path
#include "cef_render_process_handler.h" // Include for CefRenderProcessHandler
#include "launcher_client.h" // Include LauncherClient header
#include "game_launcher/core/ipc_service.h" // Include for IIPCService
#include <windows.h> // For HWND
#include <memory> // For std::unique_ptr and std::shared_ptr
#include <atomic> // For std::atomic

// Forward declarations
namespace game_launcher::core {
    class IIPCService;
}

namespace game_launcher {
namespace cef_integration {

// Implementation of CefApp and CefBrowserProcessHandler for the game launcher.
// This class handles process-level callbacks.
class LauncherApp : public CefApp,
                    public CefBrowserProcessHandler {
 public:
  // Constructor: Allows initialization without IPC service initially (set later).
  explicit LauncherApp(std::shared_ptr<game_launcher::core::IIPCService> ipc_service = nullptr);

  // CefApp methods:
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override;
  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;

  // Override CefApp method to modify command line args
  void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) override;

  // CefBrowserProcessHandler methods:
  void OnContextInitialized() override;

  // Method to set the parent window handle after creation
  void SetParentHWND(HWND hwnd);

  // Method to get the client instance
  CefRefPtr<LauncherClient> GetLauncherClient() const;

  // Method to set the IPC service after initial construction
  void SetIPCService(std::shared_ptr<game_launcher::core::IIPCService> service);

  // Called when the last browser window is closed.
  void NotifyShutdown();

  // Check if the application is currently shutting down.
  bool IsShuttingDown() const;

 private:
  // Reference to the core IPC service.
  std::shared_ptr<game_launcher::core::IIPCService> ipc_service_;

  // Handle to the parent native window
  HWND parent_hwnd_ = nullptr;

  // Instance of our custom CefClient implementation.
  // Created in OnContextInitialized.
  CefRefPtr<LauncherClient> client_instance_;

  // Instance of the render process handler.
  CefRefPtr<CefRenderProcessHandler> render_process_handler_;

  // Flag to indicate if CEF shutdown has started.
  std::atomic<bool> is_shutting_down_ = false;

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(LauncherApp);

  // Disallow copy construction and assignment.
  LauncherApp(const LauncherApp&) = delete;
  LauncherApp& operator=(const LauncherApp&) = delete;
};

} // namespace cef_integration
} // namespace game_launcher

#endif // GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_APP_H_
