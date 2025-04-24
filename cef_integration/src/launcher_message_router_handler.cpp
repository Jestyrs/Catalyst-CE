#include "game_launcher/cef_integration/launcher_message_router_handler.h"

#include <string>
#include <utility> // For std::move
#include <cstdint> // For int64_t
#include <optional> // Include for std::optional
#include <fstream>  // For reading games.json (if needed, seems placeholder now)
#include <sstream>  // For reading file stream into string
#include <iostream> // For std::cerr (consider replacing with LOG if appropriate)

#include "nlohmann/json.hpp" // For JSON parsing/serialization
#include "absl/strings/str_split.h"
#include "absl/strings/str_cat.h"
#include "absl/types/span.h"

// CEF Includes
#include "include/base/cef_logging.h" // For LOG, DLOG
#include "include/cef_parser.h"       // For CefParseJSON (though nlohmann is used more here)
#include "include/cef_values.h"       // For CefListValue
#include "include/cef_task.h"         // For CefPostTask
#include "include/base/cef_bind.h"    // For base::BindOnce
#include "wrapper/cef_helpers.h"    // For CEF_REQUIRE_UI_THREAD

// Core Includes
#include "game_launcher/core/ipc_service.h"
#include "game_launcher/core/game_status.h" // For GameStateToString
#include "game_launcher/core/user_profile.h" // Needed for UserProfile struct
#include "game_launcher/core/auth_status.h"  // Needed for AuthStatus enum and AuthStatusToString
#include "game_launcher/core/app_settings.h" // Needed for AppSettings struct and JSON conversion
#include "game_launcher/core/user_settings.h" // Added include for AppSettings

// Utilities
#include "game_launcher/cef_integration/cef_json_utils.h" // For JsonToCefDictionary (if used)

// Define common error codes used in callbacks
// Consider moving these to a shared header if used elsewhere
constexpr int ERR_IPC_SERVICE_UNAVAILABLE = -1001;
constexpr int ERR_INTERNAL_ERROR = -1002;
constexpr int ERR_INVALID_PARAMS = -32602; // JSON-RPC Invalid Params
constexpr int ERR_PARSE_ERROR = -32700;    // JSON-RPC Parse Error
constexpr int ERR_UNKNOWN_ERROR = -32000;   // JSON-RPC Server error (generic)


namespace game_launcher {
namespace cef_integration {

// Helper function to send a success JSON response
void SendJsonResponse(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback, const nlohmann::json& response) {
    if (callback) {
        callback->Success(response.dump());
    }
}

// Helper function to send an error JSON response adhering closer to JSON-RPC style
void SendErrorResponse(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback, const std::string& error_message, int error_code = ERR_INTERNAL_ERROR) {
     if (callback) {
        nlohmann::json error_response;
        error_response["error"]["code"] = error_code;
        error_response["error"]["message"] = error_message;
        // Send the JSON string representation of the error object in the failure message
        callback->Failure(error_code, error_response.dump());
     }
}

// --- Constructor ---
LauncherMessageRouterHandler::LauncherMessageRouterHandler(std::shared_ptr<core::IIPCService> ipc_service, CefRefPtr<CefBrowser> browser)
    : ipc_service_(std::move(ipc_service)), browser_(browser), weak_ptr_factory_(this) {
    LOG(INFO) << "LauncherMessageRouterHandler created for browser ID: " << (browser ? browser->GetIdentifier() : -1);
    DCHECK(ipc_service_); // Ensure IPC service is valid
    LOG(INFO) << "LauncherMessageRouterHandler created and associated with IPC service.";
    ipc_service_->AddStatusListener(this);
}

// --- Destructor ---
LauncherMessageRouterHandler::~LauncherMessageRouterHandler() {
    LOG(INFO) << "LauncherMessageRouterHandler destroyed. Unregistering listener.";
    if (ipc_service_) { // Check if service still exists
        ipc_service_->RemoveStatusListener(this);
    }
}

// --- CefMessageRouterBrowserSide::Handler Implementations ---
bool LauncherMessageRouterHandler::OnQuery(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         int64_t query_id,
                                         const CefString& request,
                                         bool persistent,
                                         CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    // Log entry point
    LOG(INFO) << "OnQuery (ID: " << query_id << ") Request: " << request.ToString();
    CEF_REQUIRE_UI_THREAD();

    // Store callback if persistent (though not used in this example logic)
    if (persistent) {
        callback_map_[query_id] = callback;
    }

    const std::string request_str = request.ToString();
    std::string message_name;
    std::string json_payload;

    // --- Parse Request ---
    const size_t colon_pos = request_str.find(':');
    if (request_str == "ping") { // Handle simple ping separately
        message_name = "ping";
        json_payload = ""; // No payload for ping
    } else if (colon_pos != std::string::npos) {
        message_name = request_str.substr(0, colon_pos);
        json_payload = request_str.substr(colon_pos + 1);
    } else {
        LOG(ERROR) << "Invalid query format (ID: " << query_id << "). Request: " << request_str;
        SendErrorResponse(callback, "Invalid query format or unknown simple request.", 0); // Use 0 or a specific code
        if (persistent) callback_map_.erase(query_id);
        return true; // Handled (by rejecting)
    }

    LOG(INFO) << "Parsed Query (ID: " << query_id << ") Name: '" << message_name << "'"; // Payload logging removed for brevity/security

    // --- Dispatch Request ---
    // Using a series of if/else if. A map lookup could also be used for larger numbers of messages.
    try {
        if (!ipc_service_) {
            LOG(ERROR) << "IPC Service is unavailable, cannot handle request: " << message_name;
            SendErrorResponse(callback, "Core service unavailable", ERR_IPC_SERVICE_UNAVAILABLE);
        } else if (message_name == "ping") {
            SendJsonResponse(callback, {{"response", "pong"}});
        } else if (message_name == "getVersionRequest") {
            HandleGetVersion(callback);
        } else if (message_name == "gameActionRequest") {
            HandleGameAction(json_payload, callback);
        } else if (message_name == "getGameListRequest") {
            HandleGetGameList(callback);
        } else if (message_name == "loginRequest") {
            HandleLogin(json_payload, callback);
        } else if (message_name == "logoutRequest") {
            HandleLogout(callback);
        } else if (message_name == "getAuthStatusRequest") {
            HandleGetAuthStatus(callback);
        } else if (message_name == "getAppSettingsRequest") {
            HandleGetAppSettings(callback);
        } else if (message_name == "setAppSettingsRequest") {
            HandleSetAppSettings(json_payload, callback);
        } else if (message_name == "requestLaunch") { // Add this case
            HandleLaunchRequest(json_payload, callback);
        } else {
            LOG(WARNING) << "Unknown message name received (ID: " << query_id << "): '" << message_name << "'";
            SendErrorResponse(callback, "Unknown message name: " + message_name, 0); // Use 0 or a specific code
        }
    } catch (const nlohmann::json::parse_error& e) {
        std::string error_msg = absl::StrCat("JSON Parse Error: ", e.what());
        LOG(ERROR) << error_msg << ", Payload: " << json_payload;
        SendErrorResponse(callback, error_msg, ERR_PARSE_ERROR);
    } catch (const nlohmann::json::exception& e) {
        // Catches errors like accessing non-existent keys or type mismatches during get<T>()
        std::string error_msg = absl::StrCat("JSON Processing Error: ", e.what());
        LOG(ERROR) << error_msg << ", Payload: " << json_payload;
        SendErrorResponse(callback, error_msg, ERR_INVALID_PARAMS);
    } catch (const std::exception& e) {
        std::string error_msg = absl::StrCat("Standard Exception: ", e.what());
        LOG(ERROR) << "Standard exception during OnQuery (ID: " << query_id << ", Name: " << message_name << "): " << e.what();
        SendErrorResponse(callback, error_msg, ERR_INTERNAL_ERROR);
    } catch (...) {
        LOG(ERROR) << "Unknown exception during OnQuery (ID: " << query_id << ", Name: " << message_name << ")";
        SendErrorResponse(callback, "An unknown internal error occurred.", ERR_UNKNOWN_ERROR);
    }

    // Clean up non-persistent callback reference
    if (!persistent && callback_map_.count(query_id)) {
         callback_map_.erase(query_id);
    } else if (persistent) {
         // If persistent, responsibility for removing the callback lies elsewhere
         // (e.g., OnQueryCanceled or when the operation completes)
    }

    return true; // Indicate the query was handled (successfully or with an error)
}


// --- Individual Request Handler Implementations ---

void LauncherMessageRouterHandler::HandleGetVersion(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    absl::StatusOr<std::string> version_result = ipc_service_->GetVersion();
    if (version_result.ok()) {
        SendJsonResponse(callback, {{"version", *version_result}});
        LOG(INFO) << "Handled getVersionRequest successfully. Version: " << *version_result;
    } else {
        LOG(ERROR) << "Failed to get version from IPC service: " << version_result.status();
        SendErrorResponse(callback, version_result.status().ToString(), static_cast<int>(version_result.status().code()));
    }
}

void LauncherMessageRouterHandler::HandleGameAction(const std::string& json_payload, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    // Parse payload first
    nlohmann::json payload = nlohmann::json::parse(json_payload); // Throws on parse error
    // Validate payload structure
    if (!payload.contains("game_id") || !payload["game_id"].is_string() ||
        !payload.contains("action") || !payload["action"].is_string()) {
        SendErrorResponse(callback, "Invalid payload: 'game_id' (string) and 'action' (string) required.", ERR_INVALID_PARAMS);
        return;
    }

    std::string game_id = payload.at("game_id").get<std::string>();
    std::string action = payload.at("action").get<std::string>();
    LOG(INFO) << "Attempting action '" << action << "' via IPC for game ID: '" << game_id << "'";

    absl::Status action_status;

    // Dispatch to appropriate IPC service method
    if (action == "launch") {
        action_status = ipc_service_->RequestLaunch(game_id);
    } else if (action == "install") {
         action_status = ipc_service_->RequestInstall(game_id); // Assuming RequestInstall exists
    } else if (action == "update") {
        action_status = ipc_service_->RequestUpdate(game_id);
    } else if (action == "cancel") {
         action_status = ipc_service_->RequestCancel(game_id); // Assuming RequestCancel exists
    } else {
        LOG(ERROR) << "Unknown action type '" << action << "' received for game ID '" << game_id << "'";
        action_status = absl::InvalidArgumentError("Unknown action type");
    }

    // Send response based on status
    if (action_status.ok()) {
        LOG(INFO) << "IPC service successfully processed action '" << action << "' for game ID '" << game_id << "'";
        SendJsonResponse(callback, {{"status", action + " initiated for " + game_id}});
    } else {
        LOG(ERROR) << "IPC service failed action '" << action << "' for game ID '" << game_id << "': " << action_status;
        SendErrorResponse(callback, action_status.ToString(), static_cast<int>(action_status.code()));
    }
}

void LauncherMessageRouterHandler::HandleGetGameList(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
     // TODO: Replace placeholder with actual call to ipc_service_
     // Example:
     // auto status_or_list = ipc_service_->GetGameList(); // Assuming such a method exists
     // if (status_or_list.ok()) {
     //     nlohmann::json games_json_list = *status_or_list; // Assuming it returns JSON or vector<GameInfo>
     //     SendJsonResponse(callback, games_json_list);
     // } else {
     //     SendErrorResponse(callback, status_or_list.status().ToString(), static_cast<int>(status_or_list.status().code()));
     // }

     // Placeholder:
     nlohmann::json game_list_json = nlohmann::json::parse(R"([
        {"id": "game1", "name": "Cyber Odyssey", "status": "NotInstalled"},
        {"id": "game2", "name": "Stellar Conquest", "status": "ReadyToPlay"},
        {"id": "game3", "name": "Mystic Realms", "status": "UpdateRequired"}
     ])");
     LOG(INFO) << "Handling getGameListRequest. Responding with placeholder data.";
     SendJsonResponse(callback, game_list_json);
}

void LauncherMessageRouterHandler::HandleLogin(const std::string& json_payload, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    nlohmann::json payload = nlohmann::json::parse(json_payload); // Throws on parse error
    if (!payload.contains("username") || !payload["username"].is_string() ||
        !payload.contains("password") || !payload["password"].is_string()) {
         SendErrorResponse(callback, "Invalid payload: 'username' and 'password' must be strings.", ERR_INVALID_PARAMS);
        return;
    }

    std::string username = payload.at("username").get<std::string>();
    std::string password = payload.at("password").get<std::string>();
    LOG(INFO) << "Attempting login via IPC for user: '" << username << "'";

    absl::Status login_status = ipc_service_->Login(username, password);
    if (login_status.ok()) {
        LOG(INFO) << "IPC service successfully processed login request for user '" << username << "'";
        SendJsonResponse(callback, {{"status", "success"}});
        // TODO: Potentially include user profile in response
    } else {
        LOG(ERROR) << "IPC service failed login for user '" << username << "': " << login_status;
        SendErrorResponse(callback, login_status.ToString(), static_cast<int>(login_status.code()));
    }
}

void LauncherMessageRouterHandler::HandleLogout(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    LOG(INFO) << "Attempting logout via IPC.";
    absl::Status logout_status = ipc_service_->Logout();
    if (logout_status.ok()) {
        LOG(INFO) << "IPC service successfully processed logout request.";
        SendJsonResponse(callback, {{"status", "success"}});
    } else {
        LOG(ERROR) << "IPC service failed logout: " << logout_status;
        SendErrorResponse(callback, logout_status.ToString(), static_cast<int>(logout_status.code()));
    }
}

void LauncherMessageRouterHandler::HandleGetAuthStatus(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    nlohmann::json response_json;
    core::AuthStatus status = ipc_service_->GetAuthStatus();
    response_json["status"] = game_launcher::core::AuthStatusToString(status);

    if (status == core::AuthStatus::kLoggedIn) {
        absl::StatusOr<core::UserProfile> profile_status = ipc_service_->GetCurrentUserProfile();
        if (profile_status.ok()) {
            response_json["profile"] = *profile_status; // Uses NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE
        } else {
            LOG(WARNING) << "User is logged in but failed to get profile: " << profile_status.status();
            response_json["profile"] = nullptr;
        }
    }
    SendJsonResponse(callback, response_json);
}

void LauncherMessageRouterHandler::HandleGetAppSettings(CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    core::AppSettings settings = ipc_service_->GetAppSettings(); // Added core:: namespace
    try {
        nlohmann::json settings_json = settings; // Uses NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE
        SendJsonResponse(callback, settings_json);
    } catch (const nlohmann::json::exception& e) {
        LOG(ERROR) << "Failed to serialize AppSettings to JSON: " << e.what();
        SendErrorResponse(callback, absl::StrCat("Internal Error: Failed to serialize settings: ", e.what()), ERR_INTERNAL_ERROR);
    }
}

void LauncherMessageRouterHandler::HandleSetAppSettings(const std::string& json_payload, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    LOG(INFO) << "Handling setAppSettingsRequest";
    // Parse and deserialize the JSON payload into AppSettings
    nlohmann::json payload = nlohmann::json::parse(json_payload); // Throws on parse error
    core::AppSettings settings = payload.get<core::AppSettings>(); // Added core:: namespace

    // Call the IPC service method
    absl::Status set_status = ipc_service_->SetAppSettings(settings);

    if (set_status.ok()) {
        LOG(INFO) << "Successfully set app settings via IPC.";
        SendJsonResponse(callback, {{"status", "Settings saved successfully."}});
    } else {
        LOG(ERROR) << "IPC service failed to set app settings: " << set_status;
        SendErrorResponse(callback, set_status.ToString(), static_cast<int>(set_status.code()));
    }
}

void LauncherMessageRouterHandler::HandleLaunchRequest(const std::string& json_payload,
                                                     CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    LOG(INFO) << "Handling requestLaunch...";
    nlohmann::json payload = nlohmann::json::parse(json_payload);

    // Extract game ID
    if (!payload.contains("gameId") || !payload["gameId"].is_string()) {
        SendErrorResponse(callback, "Invalid parameters: Missing or invalid 'gameId'", ERR_INVALID_PARAMS);
        LOG(ERROR) << "HandleLaunchRequest failed: Missing or invalid gameId.";
        return;
    }
    std::string game_id = payload["gameId"].get<std::string>();

    // Call the Core IPC Service
    absl::Status status = ipc_service_->RequestLaunch(game_id);

    if (status.ok()) {
        LOG(INFO) << "RequestLaunch successful for game ID: " << game_id;
        // Send a minimal success response. The UI will update based on the async status event.
        SendJsonResponse(callback, nlohmann::json::object()); 
    } else {
        std::string error_msg = absl::StrCat("Failed to request launch for game '", game_id, "': ", status.ToString());
        LOG(ERROR) << error_msg;
        // Map Abseil status codes to JSON-RPC style error codes if desired, 
        // or use a generic internal error code.
        SendErrorResponse(callback, error_msg, ERR_INTERNAL_ERROR); 
    }
}


// --- Binary OnQuery (Not Implemented) ---
bool LauncherMessageRouterHandler::OnQuery(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         int64_t query_id,
                                         CefRefPtr<const CefBinaryBuffer> request,
                                         bool persistent,
                                         CefRefPtr<CefMessageRouterBrowserSide::Callback> callback) {
    CEF_REQUIRE_UI_THREAD();
    LOG(WARNING) << "LauncherMessageRouterHandler::OnQuery (Binary) received query_id: " << query_id
                 << ". Binary requests are not currently implemented or handled.";
    return false; // Not handled
}

// --- OnQueryCanceled ---
void LauncherMessageRouterHandler::OnQueryCanceled(CefRefPtr<CefBrowser> browser,
                                                 CefRefPtr<CefFrame> frame,
                                                 int64_t query_id) {
    CEF_REQUIRE_UI_THREAD();
    LOG(WARNING) << "Query canceled, ID: " << query_id;
    // Remove the callback from the map if it exists
    callback_map_.erase(query_id);
    // TODO: Add logic to signal cancellation to ongoing core operations if needed.
}

// --- core::IGameStatusListener Implementation ---

// Task definition remains nested
class LauncherMessageRouterHandler::StatusUpdateTask : public CefTask {
public:
    LauncherMessageRouterHandler::StatusUpdateTask(base::WeakPtr<LauncherMessageRouterHandler> handler_weak_ptr,
                     const core::GameStatusUpdate& update)
        : handler_weak_ptr_(handler_weak_ptr),
          update_(update) {}

    void LauncherMessageRouterHandler::StatusUpdateTask::Execute() {
        CEF_REQUIRE_UI_THREAD();
        if (auto handler = handler_weak_ptr_.get()) { // Check if handler still exists
            handler->ProcessGameStatusUpdateUIImpl(update_);
        } else {
            LOG(WARNING) << "LauncherMessageRouterHandler destroyed before StatusUpdateTask could run.";
        }
    }

private:
    base::WeakPtr<LauncherMessageRouterHandler> handler_weak_ptr_;
    core::GameStatusUpdate update_; // Copy the update data

    IMPLEMENT_REFCOUNTING(StatusUpdateTask);
    DISALLOW_COPY_AND_ASSIGN(StatusUpdateTask);
};

void LauncherMessageRouterHandler::OnGameStatusUpdate(const core::GameStatusUpdate& update) {
    // Basic logging
    LOG(INFO) << "Received GameStatusUpdate for game '" << update.game_id << "' Status: " << core::GameStateToString(update.current_state);

    // Ensure execution on the UI thread
    if (!CefCurrentlyOn(TID_UI)) {
        CefPostTask(TID_UI, new StatusUpdateTask(weak_ptr_factory_.GetWeakPtr(), update));
    } else {
        ProcessGameStatusUpdateUIImpl(update);
    }
}

// This function MUST run on the UI thread.
void LauncherMessageRouterHandler::ProcessGameStatusUpdateUIImpl(const core::GameStatusUpdate& update) {
    CEF_REQUIRE_UI_THREAD();
    DLOG(INFO) << "Processing game status update on UI thread for game: " << update.game_id;

    if (!browser_) {
        LOG(ERROR) << "Cannot send game status update to UI, browser handle is null.";
        return;
    }
    CefRefPtr<CefFrame> main_frame = browser_->GetMainFrame();
    if (!main_frame) {
         LOG(ERROR) << "Cannot send process message, main frame is not available.";
         return;
    }

    // Serialize the update to JSON
    nlohmann::json update_json;
    try {
        update_json["game_id"] = update.game_id;
        update_json["status"] = core::GameStateToString(update.current_state);
        update_json["progress"] = update.progress_percent.value_or(-1); // Send -1 if nullopt
        update_json["message"] = update.message.value_or("");
        std::string json_string = update_json.dump();

        // Send update via JavaScript execution
        // Use Cef V8 value creation for potentially better performance/robustness?
        // Or just execute simple JS. Simpler for now:
        std::string js_code = "if (window.gameLauncherAPI && window.gameLauncherAPI.onStatusUpdate) {";
        js_code += "window.gameLauncherAPI.onStatusUpdate(" + json_string + ");";
        js_code += "} else { console.error('window.gameLauncherAPI.onStatusUpdate not found!'); }";

        main_frame->ExecuteJavaScript(js_code, main_frame->GetURL(), 0);
        LOG(INFO) << "Executed JS for game status update: " << update.game_id;

    } catch (const nlohmann::json::exception& e) {
        LOG(ERROR) << "Failed to serialize GameStatusUpdate to JSON: " << e.what();
    }
}

// --- CefLifeSpanHandler method (called by owning ClientHandler) ---
void LauncherMessageRouterHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
     CEF_REQUIRE_UI_THREAD();
     LOG(INFO) << "OnBeforeClose notification received for browser ID: " << browser->GetIdentifier();
     if (browser_ && browser_->IsSame(browser)) {
         // Release the browser reference.
         browser_ = nullptr;
         LOG(INFO) << "Browser instance reference cleared.";
         weak_ptr_factory_.InvalidateWeakPtrs();
         DLOG(INFO) << "Weak pointers invalidated.";
         callback_map_.clear();
         DLOG(INFO) << "Callback map cleared.";
     }
}

// --- CefBaseRefCounted Implementations ---
void LauncherMessageRouterHandler::AddRef() const {
  ref_count_.Increment();
}

bool LauncherMessageRouterHandler::Release() const {
  if (!ref_count_.Decrement()) {
    // Note: Ensure this handler is managed appropriately (e.g., not stack allocated)
    // if manual deletion is required.
    delete this;
    return true;
  }
  return false;
}

bool LauncherMessageRouterHandler::HasOneRef() const {
  return ref_count_.IsOne();
}

bool LauncherMessageRouterHandler::HasAtLeastOneRef() const {
  return !ref_count_.IsZero();
}

} // namespace cef_integration
} // namespace game_launcher