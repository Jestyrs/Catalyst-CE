// cef_integration/src/launcher_render_process_handler.cpp
#include "game_launcher/cef_integration/launcher_render_process_handler.h"
#include "base/cef_logging.h" // Assuming correct path
#include "wrapper/cef_helpers.h"
#include "wrapper/cef_message_router.h"
// #include "cef_app.h" // Maybe needed depending on LOG/CHECK macros
// #include "cef_v8.h" // Included but not directly used here

namespace game_launcher::cef_integration { // C++17 nested namespace syntax

LauncherRenderProcessHandler::LauncherRenderProcessHandler() {
    // LOG(INFO) << "LauncherRenderProcessHandler created.";
}

LauncherRenderProcessHandler::~LauncherRenderProcessHandler() {
    // LOG(INFO) << "LauncherRenderProcessHandler destroyed.";
}

void LauncherRenderProcessHandler::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                                CefRefPtr<CefFrame> frame,
                                                CefRefPtr<CefV8Context> context) {
    CEF_REQUIRE_RENDERER_THREAD(); // Macro call - semicolon likely included in macro
    LOG(INFO) << "Render process V8 context created for frame: " << frame->GetIdentifier(); // Ends with ;

    // Create the renderer-side router for query handling if it doesn't exist.
    if (!message_router_) {
        CefMessageRouterConfig config; // Ends with ;
        message_router_ = CefMessageRouterRendererSide::Create(config); // Ends with ;
        LOG(INFO) << "Renderer-side message router created."; // Ends with ;
    }

    // Give the message router an opportunity to attach handlers to the context.
    // This is what injects cefQuery, cefQueryCancel etc.
    message_router_->OnContextCreated(browser, frame, context); // Ends with ;
} // Correct closing brace for OnContextCreated

void LauncherRenderProcessHandler::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                                 CefRefPtr<CefFrame> frame,
                                                 CefRefPtr<CefV8Context> context) {
    // CEF_REQUIRE_RENDERER_THREAD(); // Commented out
    LOG(INFO) << "Render process V8 context released for frame: " << frame->GetIdentifier(); // Ends with ;

    // Give the message router an opportunity to release handlers.
    if (message_router_) {
        message_router_->OnContextReleased(browser, frame, context); // Ends with ;
    }
} // Correct closing brace for OnContextReleased

bool LauncherRenderProcessHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
    // Check if the message is the cefQuery message
    if (message->GetName() == "cefQueryMsg") {
        //LOG(INFO) << "Render process received message: " << message->GetName();
        if (message_router_) {
            //LOG(INFO) << "Attempting to handle message '" << message->GetName() 
            //          << "' with renderer-side router.";
            bool handled = message_router_->OnProcessMessageReceived(browser, frame, source_process, message);
            if (handled) {
                //LOG(INFO) << "Message '" << message->GetName() 
                //          << "' was handled by renderer-side router.";
                return true;
            } else {
                //LOG(WARNING) << "Message '" << message->GetName() 
                //           << "' was NOT handled by renderer-side router.";
                return false;
            }
        } else {
            LOG(ERROR) << "Renderer-side message router (message_router_) is NULL when trying to handle '" 
                       << message->GetName() << "'. Message cannot be processed.";
            return false; // Indicate message not handled
        }
    }
    // Return false if the message is not one we specifically handle here.
    return false; 
}

} // namespace game_launcher::cef_integration // Correct closing brace for namespace(s)

// If using separate namespaces:
// } // namespace cef_integration
// } // namespace game_launcher