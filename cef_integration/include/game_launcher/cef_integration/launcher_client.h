#ifndef GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_CLIENT_H_
#define GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_CLIENT_H_

#include "cef_client.h" // Base CEF client (Reverted)
#include "cef_life_span_handler.h" // Lifespan handler (Reverted)
#include "wrapper/cef_message_router.h" // (Reverted)
// #include "cef_render_handler.h" // No longer needed for windowed mode
#include "cef_context_menu_handler.h" // Context menu handler (Reverted)
#include "cef_load_handler.h" // Load handler (Reverted)
#include "game_launcher/core/ipc_service.h" // Use the interface
#include "cef_render_process_handler.h" // Include for Render Process Handler
#include "launcher_message_router_handler.h"
#include <memory> // For std::unique_ptr
#include <list> // For std::list

// Forward declare LauncherApp
namespace game_launcher { namespace cef_integration { class LauncherApp; } }

namespace game_launcher::cef_integration {

// Implementation of CefClient for the game launcher browser process.
// This class handles callbacks related to the browser window and communication.
class LauncherClient : public CefClient,
                       public CefLifeSpanHandler,
                       public CefLoadHandler, // Add CefLoadHandler
                       public CefContextMenuHandler { // Add Context Menu Handler
 public:
    // Constructor now takes the IPC service shared pointer and LauncherApp reference
    explicit LauncherClient(std::shared_ptr<core::IIPCService> ipc_service, 
                            CefRefPtr<LauncherApp> launcher_app);
    ~LauncherClient() override;

    // --- CefClient overrides --- 
    // Return this object to handle lifespan events.
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
    // Return this object to handle context menu events.
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override;
    // Return this object to handle load events.
    CefRefPtr<CefLoadHandler> GetLoadHandler() override;

    // Return the message router browser-side handler.
    CefRefPtr<CefMessageRouterBrowserSide> GetMessageRouter() const;

    // Return the specific message handler.
    LauncherMessageRouterHandler* GetMessageHandler() const;

    // Need to return other handlers if implemented (e.g., DisplayHandler, RequestHandler)

    // Override this method to handle process messages from the render process.
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) override; // Declare override

    // Method to receive the IPC service pointer from LauncherApp
    void SetIPCService(std::shared_ptr<core::IIPCService> service);

    // Getters
    CefRefPtr<CefBrowser> GetBrowser() const;

    // --- CefLifeSpanHandler overrides --- 
    // Called after a new browser window is created.
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    // Called when a browser window is about to close.
    // This is where we can call CefQuitMessageLoop to exit the application.
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    // Called before a browser window is closed.
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // --- CefContextMenuHandler overrides --- 
    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             CefRefPtr<CefContextMenuParams> params,
                             CefRefPtr<CefMenuModel> model) override;

    bool OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefContextMenuParams> params,
                              int command_id,
                              EventFlags event_flags) override;

    // --- CefLoadHandler overrides --- 
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode, const CefString& errorText,
                     const CefString& failedUrl) override;

 private:
    // The message router browser-side handler.
    CefRefPtr<CefMessageRouterBrowserSide> message_router_;         // Message router instance
    // Handler for specific message types.
    std::unique_ptr<LauncherMessageRouterHandler> message_handler_; // Owns the handler

    // Reference to the core IPC service (dependency injection)
    std::shared_ptr<core::IIPCService> ipc_service_; // <<< Add member variable

    // Reference back to the main application instance.
    CefRefPtr<LauncherApp> launcher_app_; // Store LauncherApp reference

    // The single browser instance for this client.
    // Assuming only one browser per client instance in this simple app.
    CefRefPtr<CefBrowser> browser_ = nullptr;

    // Keep track of the number of browsers.
    int browser_count_ = 0;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(LauncherClient);
};

} // namespace game_launcher::cef_integration

#endif // GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_CLIENT_H_
