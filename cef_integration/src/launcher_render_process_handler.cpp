// cef_integration/src/launcher_render_process_handler.cpp
#include "game_launcher/cef_integration/launcher_render_process_handler.h"
#include "game_launcher/cef_integration/game_launcher_api_handler.h"
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
    // Ensure the message router is created (it's a member variable)
    if (!message_router_) {
        CefMessageRouterConfig config;
        message_router_ = CefMessageRouterRendererSide::Create(config);
        LOG(INFO) << "LauncherRenderProcessHandler - Created renderer-side message router.";
    }

    LOG(INFO) << "LauncherRenderProcessHandler::OnContextCreated - Entered for frame: " << frame->GetIdentifier();

    // Only inject the API into the main frame's context
    if (!frame->IsMain()) {
        LOG(INFO) << "LauncherRenderProcessHandler::OnContextCreated - Skipping non-main frame.";
        return;
    }

    LOG(INFO) << "LauncherRenderProcessHandler::OnContextCreated - Injecting API into main frame.";

    // Get the global window object
    CefRefPtr<CefV8Value> global = context->GetGlobal();
    if (!global) {
        LOG(ERROR) << "LauncherRenderProcessHandler::OnContextCreated - Failed to get V8 global object.";
        return;
    }

    // Create the V8 handler instance
    CefRefPtr<GameLauncherApiHandler> api_handler = new GameLauncherApiHandler();
    if (!api_handler) {
        LOG(ERROR) << "LauncherRenderProcessHandler::OnContextCreated - Failed to create GameLauncherApiHandler instance.";
        return;
    }

    // Create the 'gameLauncherAPI' object
    CefRefPtr<CefV8Value> api_object = CefV8Value::CreateObject(nullptr, nullptr); // Use version with null accessor/interceptor
    if (!api_object) {
        LOG(ERROR) << "LauncherRenderProcessHandler::OnContextCreated - Failed to create api_object (CefV8Value::CreateObject)." ;
        return;
    }
    LOG(INFO) << "LauncherRenderProcessHandler::OnContextCreated - api_object created successfully.";

    // Create and attach functions to the api_object
    const char* function_names[] = {"getGameList", "getAuthStatus", "getVersion"};
    for (const char* func_name : function_names) {
        CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction(func_name, api_handler);
        if (!func) {
            LOG(ERROR) << "LauncherRenderProcessHandler::OnContextCreated - Failed to create function '" << func_name << "'.";
            continue; // Skip attaching this function
        }
        bool success = api_object->SetValue(func_name, func, V8_PROPERTY_ATTRIBUTE_NONE);
        if (!success) {
             LOG(ERROR) << "LauncherRenderProcessHandler::OnContextCreated - Failed to set function '" << func_name << "' on api_object.";
        }
        LOG(INFO) << "LauncherRenderProcessHandler::OnContextCreated - Function '" << func_name << "' attached.";
    }

    // Attach the 'gameLauncherAPI' object to the window object
    bool setObjectSuccess = global->SetValue("gameLauncherAPI", api_object, V8_PROPERTY_ATTRIBUTE_NONE);
    if (!setObjectSuccess) {
        LOG(ERROR) << "LauncherRenderProcessHandler::OnContextCreated - Failed to attach gameLauncherAPI to window object!";
    } else {
        LOG(INFO) << "LauncherRenderProcessHandler::OnContextCreated - gameLauncherAPI successfully attached to window object.";
    }

    // --- Message Router Setup ---
    // Call OnContextCreated on the instance variable
    message_router_->OnContextCreated(browser, frame, context);
}

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