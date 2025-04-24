
#ifndef GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_RENDER_PROCESS_HANDLER_H_
#define GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_RENDER_PROCESS_HANDLER_H_

#include "cef_render_process_handler.h"
#include "wrapper/cef_message_router.h"

namespace game_launcher {
namespace cef_integration {

// Handles callbacks for the render process, primarily setting up message routing.
class LauncherRenderProcessHandler : public CefRenderProcessHandler {
public:
    LauncherRenderProcessHandler();
    ~LauncherRenderProcessHandler() override;

    // --- CefRenderProcessHandler Overrides ---

    // Called when a V8 context is created (e.g., for a new frame or iframe).
    // This is where we create the renderer-side message router.
    void OnContextCreated(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override;

    // Called when a V8 context is released.
    void OnContextReleased(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefV8Context> context) override;

    // Called when a process message is received from the browser process.
    // We need to delegate this to the message router.
    bool OnProcessMessageReceived(
        CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process, // Should always be PID_BROWSER
        CefRefPtr<CefProcessMessage> message) override;

private:
// Renderer-side message router instance.
CefRefPtr<CefMessageRouterRendererSide> message_router_;

// Disallow copy construction and assignment.
LauncherRenderProcessHandler(const LauncherRenderProcessHandler&) = delete;
LauncherRenderProcessHandler& operator=(const LauncherRenderProcessHandler&) = delete;

// Include the default reference counting implementation.
IMPLEMENT_REFCOUNTING(LauncherRenderProcessHandler);
};

} // namespace cef_integration
} // namespace game_launcher

#endif // GAME_LAUNCHER_CEF_INTEGRATION_LAUNCHER_RENDER_PROCESS_HANDLER_H_
