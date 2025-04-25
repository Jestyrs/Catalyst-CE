// cef_integration/src/game_launcher_api_handler.cpp
#include "game_launcher/cef_integration/game_launcher_api_handler.h"
#include "include/base/cef_logging.h"
#include "include/wrapper/cef_helpers.h"
#include <sstream> // For JSON construction (basic)

namespace game_launcher::cef_integration {

GameLauncherApiHandler::GameLauncherApiHandler() = default;

// Helper function to execute cefQuery
// Logs errors internally if V8 exceptions occur.
void GameLauncherApiHandler::ExecuteCefQuery(CefRefPtr<CefV8Context> context,
                                             const std::string& request,
                                             const CefV8ValueList& arguments) {
    // Argument check (simplified - just ensure callbacks are functions)
    if (arguments.size() < 2 || !arguments[0]->IsFunction() || !arguments[1]->IsFunction()) {
        LOG(ERROR) << "API function '" << request << "' requires two function arguments (successCallback, failureCallback).";
        // Can't easily set V8 exception here, caller should ideally validate args beforehand if needed.
        return; 
    }

    CefRefPtr<CefV8Value> global = context->GetGlobal();
    CefRefPtr<CefV8Value> cefQuery = global->GetValue("cefQuery");

    if (cefQuery.get() && cefQuery->IsFunction()) {
        CefV8ValueList queryArgs;

        // 1. The request object
        CefRefPtr<CefV8Value> requestObj = CefV8Value::CreateObject(nullptr, nullptr);
        requestObj->SetValue("request", CefV8Value::CreateString(request), V8_PROPERTY_ATTRIBUTE_NONE);
        // For now, assume requests are not persistent
        requestObj->SetValue("persistent", CefV8Value::CreateBool(false), V8_PROPERTY_ATTRIBUTE_NONE);

        // TODO: Add actual request data/payload if needed based on 'request' and 'arguments'
        // Example: if (request == "someFuncWithArgs") { ... }

        queryArgs.push_back(requestObj);

        // 2. Success Callback (Required)
        queryArgs.push_back(arguments[0]);

        // 3. Failure Callback (Required)
        queryArgs.push_back(arguments[1]);

        // Execute cefQuery using the 3-argument version
        // The second argument is the 'this' object for the JS function call.
        // Since cefQuery is likely a global function (window.cefQuery), use 'global'.
        CefRefPtr<CefV8Value> queryRetval = cefQuery->ExecuteFunctionWithContext(context, global, queryArgs);

        // NOTE: Cannot check context->HasException() / context->GetException() here due to API differences/build errors.
        // Rely on JS callbacks or browser-side message handler logs for detailed V8 execution errors.

        // We don't check the 'queryRetval' here as cefQuery itself doesn't usually return meaningful data

    } else {
        LOG(ERROR) << "window.cefQuery function not found in V8 context.";
    }
}

bool GameLauncherApiHandler::Execute(const CefString& name,
                                   CefRefPtr<CefV8Value> object,
                                   const CefV8ValueList& arguments,
                                   CefRefPtr<CefV8Value>& retval,
                                   CefString& exception) {
    CEF_REQUIRE_RENDERER_THREAD();

    // --- Begin Enhanced Logging ---
    LOG(INFO) << "---> GameLauncherApiHandler::Execute ENTERED";
    std::string functionName = name.ToString();
    LOG(INFO) << "     Function Name: " << functionName;
    LOG(INFO) << "     Argument Count: " << arguments.size();

    // Check if arguments are provided and if the expected callbacks are functions
    if (arguments.size() >= 2) {
        LOG(INFO) << "     Arg 0 Type Is Function: " << (arguments[0]->IsFunction() ? "Yes" : "No");
        LOG(INFO) << "     Arg 1 Type Is Function: " << (arguments[1]->IsFunction() ? "Yes" : "No");
    } else {
        LOG(WARNING) << "     Less than 2 arguments received, expected success/failure callbacks.";
    }
    // --- End Enhanced Logging ---

    CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
    if (!context) {
        exception = CefString("Failed to get current V8 context."); // Use CefString constructor
        LOG(ERROR) << exception.ToString();
        return true; // Indicate exception was set
    }

    LOG(INFO) << "GameLauncherApiHandler::Execute called for function: " << functionName;

    // --- Route based on function name --- 
    if (functionName == "getGameList" || 
        functionName == "getAuthStatus" || 
        functionName == "getVersion") 
    {
        // These functions simply pass the name as the request string
        // We expect arguments[0] = successCallback, arguments[1] = failureCallback
        ExecuteCefQuery(context, functionName, arguments);
        // Return true because the call was handled (async response pending)
        return true; 
    }
    // Add more 'else if' blocks here for other API functions like installGame, launchGame etc.
    // else if (functionName == "installGame") {
    //    // Potentially extract game ID from arguments[0]
    //    // Construct request string like "installGame:gameId"
    //    // ExecuteCefQuery(context, requestString, arguments[1], arguments[2]);
    // }
    else {
        // Function not recognized
        std::string errorMsg = "Unknown API function executed: ";
        errorMsg += functionName;
        exception.FromString(errorMsg); // Use FromString method for std::string
        LOG(ERROR) << errorMsg;
        return true; // Indicate exception was set
    }

    return false; // Should not be reached if all paths return true
}

} // namespace game_launcher::cef_integration
