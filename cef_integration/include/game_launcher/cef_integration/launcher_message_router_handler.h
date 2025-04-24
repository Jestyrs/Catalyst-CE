#ifndef GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_MESSAGE_ROUTER_HANDLER_H_
#define GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_MESSAGE_ROUTER_HANDLER_H_

#include "include/cef_base.h" // Corrected CEF include path
#include "include/wrapper/cef_message_router.h" // Corrected CEF include path
#include "include/wrapper/cef_helpers.h" // For CEF_REQUIRE_UI_THREAD
#include "game_launcher/core/ipc_service.h" // Include the core IPC service interface
#include "game_launcher/core/game_status_listener.h" // Include the listener interface
#include "base/cef_atomic_ref_count.h" // Corrected CEF include path
#include "include/cef_values.h" // For CefDictionaryValue
#include "include/cef_parser.h" // For CefParseJSON, CefWriteJSON
#include "include/base/cef_weak_ptr.h" // Include for CefWeakPtrFactory
#include "include/cef_browser.h"   // Include for CefBrowser
#include <map> // Include for std::map
#include <memory> // Include for shared_ptr

namespace game_launcher {
namespace cef_integration {

/**
 * @brief Handles JavaScript query requests and receives game status updates.
 *
 * This class processes messages sent via `window.cefQuery` from the UI's
 * JavaScript code, interacting with core::IIPCService to fulfill requests.
 * It also implements core::IGameStatusListener to receive asynchronous updates
 * about game state changes (install progress, launch status) and forwards
 * them to the JavaScript UI.
 */
class LauncherMessageRouterHandler : public CefMessageRouterBrowserSide::Handler,
                                     public core::IGameStatusListener,
                                     public CefBaseRefCounted {
public:
    /**
     * @brief Constructs the handler.
     * @param ipc_service A shared pointer to the IPC service implementation. Must outlive this handler.
     * @param browser The main browser instance this handler is associated with. Used for pushing status updates.
     */
    explicit LauncherMessageRouterHandler(std::shared_ptr<core::IIPCService> ipc_service, CefRefPtr<CefBrowser> browser);

    /**
     * @brief Destructor. Unregisters from the IPC service.
     */
    ~LauncherMessageRouterHandler();

    // --- core::IGameStatusListener Overrides ---

    /**
     * @brief Called by the core::IIPCService when a game's status changes.
     *
     * This method receives the update, formats it as a JavaScript call,
     * and schedules its execution on the CEF UI thread.
     *
     * @param update The status update details.
     */
    void OnGameStatusUpdate(const core::GameStatusUpdate& update) override;

    // --- CefMessageRouterBrowserSide::Handler Overrides ---

    /**
     * @brief Called when a JavaScript query (string-based) is received.
     *
     * Expects requests in the format "message_name:json_payload".
     * Handles known message names like "getVersionRequest" and "launchGameRequest".
     *
     * @param browser The originating browser instance.
     * @param frame The originating frame.
     * @param query_id A unique ID for this query.
     * @param request The string request data ("message_name:payload").
     * @param persistent True if the query is persistent.
     * @param callback Used to send the response (success or failure) back to JavaScript.
     * @return True if the query was handled, false otherwise.
     */
    bool OnQuery(CefRefPtr<CefBrowser> browser,
                 CefRefPtr<CefFrame> frame,
                 int64_t query_id,
                 const CefString& request,
                 bool persistent,
                 CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) override;

    /**
     * @brief Called when a JavaScript query (binary-based) is received.
     *
     * Currently not implemented; logs a warning and returns false.
     *
     * @param browser The originating browser instance.
     * @param frame The originating frame.
     * @param query_id A unique ID for this query.
     * @param request The binary request data.
     * @param persistent True if the query is persistent.
     * @param callback Used to send the response (success or failure) back to JavaScript.
     * @return True if the query was handled, false otherwise.
     */
    bool OnQuery(CefRefPtr<CefBrowser> browser,
                 CefRefPtr<CefFrame> frame,
                 int64_t query_id,
                 CefRefPtr<const CefBinaryBuffer> request,
                 bool persistent,
                 CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) override;

    /**
     * @brief Called when a JavaScript query is canceled (e.g., page navigation).
     *
     * @param browser The originating browser instance.
     * @param frame The originating frame.
     * @param query_id The ID of the query that was canceled.
     */
    void OnQueryCanceled(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int64_t query_id) override;

    // --- CefLifeSpanHandler method (called by owning ClientHandler) ---
    // NOTE: Removed 'override' as this class doesn't directly inherit CefLifeSpanHandler
    void OnBeforeClose(CefRefPtr<CefBrowser> browser);

    // --- CefBaseRefCounted Overrides (Declarations) ---
    void AddRef() const override;
    bool Release() const override;
    bool HasOneRef() const override;
    bool HasAtLeastOneRef() const override;

private:
    // Forward declare nested class
    class StatusUpdateTask;
    friend class StatusUpdateTask; // Add friend declaration

    // Add the private forward declaration back
    void ProcessGameStatusUpdateUIImpl(const core::GameStatusUpdate& update);

    // Core service dependency.
    std::shared_ptr<core::IIPCService> ipc_service_;

    // The browser window this handler is associated with.
    CefRefPtr<CefBrowser> browser_;

    // Map to store active query callbacks.
    std::map<int64_t, CefRefPtr<CefMessageRouterBrowserSide::Callback>> callback_map_;

    // Factory for creating weak pointers to this object, used for safe async calls.
    base::WeakPtrFactory<LauncherMessageRouterHandler> weak_ptr_factory_{this};

    // Reference count for managing object lifetime manually.
    // Mutable allows modification within const methods (AddRef, Release, etc.)
    mutable base::AtomicRefCount ref_count_;

    // Handler functions for specific JS requests
    void HandleGetVersion(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);
    void HandleGameAction(const std::string& json_payload, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);
    void HandleGetGameList(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);
    void HandleLogin(const std::string& json_payload, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);
    void HandleLogout(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);
    void HandleGetAuthStatus(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);
    void HandleGetAppSettings(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);
    void HandleSetAppSettings(const std::string& json_payload, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);
    void HandleLaunchRequest(const std::string& json_payload, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback);

    // Helper to send status updates to the UI thread
    void SendStatusUpdateToUI(const std::vector<core::GameStatusUpdate>& updates);

    // Disallow copy and assign operations.
    DISALLOW_COPY_AND_ASSIGN(LauncherMessageRouterHandler);
};

} // namespace cef_integration
} // namespace game_launcher

#endif // GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_MESSAGE_ROUTER_HANDLER_H_