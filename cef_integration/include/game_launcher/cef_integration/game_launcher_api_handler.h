// cef_integration/include/game_launcher/cef_integration/game_launcher_api_handler.h
#ifndef GAME_LAUNCHER_CEF_INTEGRATION_GAME_LAUNCHER_API_HANDLER_H_
#define GAME_LAUNCHER_CEF_INTEGRATION_GAME_LAUNCHER_API_HANDLER_H_

#include "cef_v8.h"

namespace game_launcher::cef_integration {

// Handles V8 function calls originating from the gameLauncherAPI object in JavaScript.
class GameLauncherApiHandler : public CefV8Handler {
public:
    GameLauncherApiHandler();

    // Override from CefV8Handler.
    // This method is called when a JavaScript function bound to this handler is executed.
    bool Execute(const CefString& name,          // Function name called
                 CefRefPtr<CefV8Value> object,  // The JS object the function was called on
                 const CefV8ValueList& arguments, // Arguments passed from JS
                 CefRefPtr<CefV8Value>& retval,   // Return value (if any)
                 CefString& exception) override; // Exception string (if any)

private:
    // Helper function to simplify executing cefQuery for API calls
    // MODIFIED: Added payloadJson parameter to match .cpp definition
    void ExecuteCefQuery(CefRefPtr<CefV8Context> context,
                         const std::string& request,
                         const CefV8ValueList& arguments,
                         const std::string& payloadJson = "{}"); // Default value included

    IMPLEMENT_REFCOUNTING(GameLauncherApiHandler);
    // Disallow copy construction and assignment.
    GameLauncherApiHandler(const GameLauncherApiHandler&) = delete;
    GameLauncherApiHandler& operator=(const GameLauncherApiHandler&) = delete;
};

} // namespace game_launcher::cef_integration

#endif // GAME_LAUNCHER_CEF_INTEGRATION_GAME_LAUNCHER_API_HANDLER_H_
