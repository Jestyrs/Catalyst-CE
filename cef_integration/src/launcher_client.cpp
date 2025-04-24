#include "include/base/cef_logging.h"

#include "game_launcher/cef_integration/launcher_client.h"

#include <string>
#include <sstream> // For std::stringstream
#include "wrapper/cef_helpers.h" // For CEF_REQUIRE_UI_THREAD etc.
#include "cef_app.h" // For CefQuitMessageLoop
#include "cef_browser.h" // For CefBrowser
#include "cef_menu_model.h" // For CefMenuModel

namespace game_launcher::cef_integration {

namespace {
    // Define command IDs for the context menu
    const int COMMAND_ID_SHOW_DEVTOOLS = MENU_ID_USER_FIRST + 1;
} // namespace

// Constructor: Takes the IPC service shared pointer and initializes the router.
LauncherClient::LauncherClient(std::shared_ptr<core::IIPCService> ipc_service)
    : ipc_service_(std::move(ipc_service)), message_router_(CefMessageRouterBrowserSide::Create(CefMessageRouterConfig())) { 
    LOG(INFO) << "LauncherClient created.";

    // Message handler will be created and added in OnAfterCreated
}

LauncherClient::~LauncherClient() {
    LOG(INFO) << "LauncherClient destroyed.";
    // Ensure handler is removed from router if it exists
    if (message_handler_ && message_router_) {
        message_router_->RemoveHandler(message_handler_.get());
    }
}

// Return the message router instance.
CefRefPtr<CefMessageRouterBrowserSide> LauncherClient::GetMessageRouter() const {
    return message_router_;
}

LauncherMessageRouterHandler* LauncherClient::GetMessageHandler() const {
    return message_handler_.get();
}

// Method to get the browser instance
CefRefPtr<CefBrowser> LauncherClient::GetBrowser() const {
    return browser_;
}

// --- CefLifeSpanHandler Implementation ---

void LauncherClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    LOG(INFO) << "+++ LauncherClient::OnAfterCreated called.";
    CEF_REQUIRE_UI_THREAD(); // Ensure this runs on the UI thread.

    LOG(INFO) << "Browser window created (ID: " << browser->GetIdentifier() << ").";

    // Store the browser pointer (assuming single browser instance)
    if (!browser_.get()) {
        browser_ = browser;
    }

    browser_count_++; // Increment browser count

    // Now that the browser exists, create the message handler and add it to the router.
    if (!message_handler_) { // Only create if it doesn't exist
        message_handler_ = std::make_unique<LauncherMessageRouterHandler>(ipc_service_, browser);
        if (message_router_) {
            message_router_->AddHandler(message_handler_.get(), false); // Add handler (not first)
            LOG(INFO) << "LauncherMessageRouterHandler created and added to router for browser ID: " << browser->GetIdentifier();
        } else {
            LOG(ERROR) << "Message router is null when trying to add handler in OnAfterCreated!";
        }
    } else {
        LOG(WARNING) << "OnAfterCreated called but message handler already exists.";
    }
}

bool LauncherClient::DoClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    LOG(INFO) << "Browser window close requested (ID: " << browser->GetIdentifier() << "). Allowing close.";
    // Allow the browser to close.
    return false; 
}

void LauncherClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    LOG(INFO) << "Browser window closing (ID: " << browser->GetIdentifier() << ").";

    // Clean up router state associated with the closing browser.
    if (message_router_) {
        message_router_->OnBeforeClose(browser);
        // Remove the handler associated with this browser window before it closes.
        if (message_handler_) {
            message_router_->RemoveHandler(message_handler_.get());
            LOG(INFO) << "LauncherMessageRouterHandler removed from router for browser ID: " << browser->GetIdentifier();
        }
    }

    // Release the handler itself (unique_ptr takes care of deletion)
    message_handler_.reset();

    browser_count_--; // Decrement browser count
    LOG(INFO) << "Browser count: " << browser_count_;

    // If this is the last browser window, quit the CEF message loop.
    if (browser_count_ == 0) {
        LOG(INFO) << "Last browser closed. Quitting message loop.";
        CefQuitMessageLoop();
    }
}

bool LauncherClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                            CefRefPtr<CefFrame> frame,
                                            CefProcessId source_process,
                                            CefRefPtr<CefProcessMessage> message) {
    CEF_REQUIRE_UI_THREAD(); // Most message router actions require the UI thread
    // LOG(INFO) << "LauncherClient::OnProcessMessageReceived"; // Can be noisy
    // Give the message router a chance to handle the message.
    if (message_router_->OnProcessMessageReceived(browser, frame, source_process, message)) {
        return true;
    }
    
    // Handle other process messages if needed

    return false;
}

// --- CefLoadHandler Implementation ---

void LauncherClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                               ErrorCode errorCode, const CefString& errorText,
                               const CefString& failedUrl) {
    CEF_REQUIRE_UI_THREAD();

    LOG(ERROR) << "!!! Browser load error: Code=" << errorCode << ", Text='" << errorText.ToString()
               << "', URL='" << failedUrl.ToString() << "'";

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a basic error message.
    // TODO: Load a more user-friendly error page?
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
          "<h2>Failed to load URL: "
       << std::string(failedUrl)
       << " with error code " << errorCode << " (" << errorText.ToString()
       << ").</h2></body></html>";
    // Load the error message using a data URI
    frame->LoadURL("data:text/html;charset=utf-8," + ss.str());
}

// --- CefContextMenuHandler Implementation ---

void LauncherClient::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame,
                                       CefRefPtr<CefContextMenuParams> params,
                                       CefRefPtr<CefMenuModel> model) {
    CEF_REQUIRE_UI_THREAD();

    // Clear the default menu
    model->Clear(); 

    // Add "Show DevTools" option
    model->AddItem(COMMAND_ID_SHOW_DEVTOOLS, "Show DevTools");
    // Disable if DevTools are already open for this browser?
    // model->SetEnabled(COMMAND_ID_SHOW_DEVTOOLS, !browser->GetHost()->HasDevTools());
}

bool LauncherClient::OnContextMenuCommand(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefRefPtr<CefContextMenuParams> params,
                                          int command_id,
                                          EventFlags event_flags) {
    CEF_REQUIRE_UI_THREAD();

    LOG(INFO) << "OnContextMenuCommand called, command_id=" << command_id;

    if (command_id == COMMAND_ID_SHOW_DEVTOOLS) {
        // Show DevTools for the browser instance.
        CefWindowInfo windowInfo;
        CefBrowserSettings settings;
#ifdef _WIN32
        windowInfo.SetAsPopup(NULL, L"GameLauncher DevTools");
#endif
        LOG(INFO) << "Opening DevTools via context menu for browser ID: " << browser->GetIdentifier();
        browser->GetHost()->ShowDevTools(windowInfo, CefRefPtr<CefClient>(), settings, CefPoint());
        return true; // Command handled
    }

    return false; // Command not handled
}

// --- CefClient Implementation ---
CefRefPtr<CefLifeSpanHandler> LauncherClient::GetLifeSpanHandler() {
    return this;
}

CefRefPtr<CefContextMenuHandler> LauncherClient::GetContextMenuHandler() {
    return this;
}

CefRefPtr<CefLoadHandler> LauncherClient::GetLoadHandler() {
    return this;
}

} // namespace game_launcher::cef_integration
