#include "include/base/cef_logging.h"
#include "game_launcher/cef_integration/launcher_app.h"
#include <filesystem>                 // For path manipulation (C++17)
#include <string>
#include <algorithm> // Added for std::replace
#include "wrapper/cef_helpers.h" // For CEF_REQUIRE_UI_THREAD etc.
#include "game_launcher/cef_integration/launcher_client.h" // Ensure client is included
#include "game_launcher/cef_integration/launcher_message_router_handler.h"
#include "game_launcher/cef_integration/launcher_render_process_handler.h" // Include the render process handler
#include "game_launcher/core/core_ipc_service.h" // Include the concrete IPC service implementation
#include "cef_browser.h" // Include for CefBrowserHost
#include "cef_app.h" // Include for CefRect
#include <windows.h>
#include <libloaderapi.h> // For GetModuleFileNameW
#include "cef_thread.h" // Include for CefGetCurrentThreadId
#include "include/cef_command_line.h" // Required for CefCommandLine
#include <atomic> // Include for std::atomic

namespace game_launcher::cef_integration {

namespace { // Use anonymous namespace for internal linkage

// Helper function to get executable path within this file
std::filesystem::path GetExecutableDir_LauncherApp() {
    wchar_t path_buf[MAX_PATH];
    // Use HMODULE = NULL to get the path of the current process executable
    if (GetModuleFileNameW(NULL, path_buf, MAX_PATH) == 0) {
        LOG(ERROR) << "Failed to get module file name. Error code: " << GetLastError();
        return {}; // Return empty path on error
    }
    return std::filesystem::path(path_buf).parent_path();
}

} // anonymous namespace

LauncherApp::LauncherApp(std::shared_ptr<core::IIPCService> ipc_service)
    : parent_hwnd_(nullptr),
      ipc_service_(std::move(ipc_service)), // Store potentially nullptr service
      client_instance_(nullptr),
      is_shutting_down_(false) { // Initialize the flag
    LOG(INFO) << "LauncherApp created.";
    render_process_handler_ = new LauncherRenderProcessHandler();
}

void LauncherApp::SetParentHWND(HWND hwnd) {
    parent_hwnd_ = hwnd;
    LOG(INFO) << "LauncherApp::SetParentHWND - Parent HWND set to: " << parent_hwnd_;
}

// Method to signal that shutdown is starting
void LauncherApp::NotifyShutdown() {
    LOG(INFO) << "LauncherApp::NotifyShutdown called.";
    is_shutting_down_.store(true);
}

// Check if the application is currently shutting down.
bool LauncherApp::IsShuttingDown() const {
    return is_shutting_down_.load();
}

// CefBrowserProcessHandler methods:
void LauncherApp::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();
    LOG(INFO) << "Browser process context initialized.";

    // Ensure parent HWND is set before creating browser
    if (!parent_hwnd_) {
        LOG(ERROR) << "Parent HWND is null in OnContextInitialized! Cannot create browser.";
        CefQuitMessageLoop();
        return;
    }

    // Ensure IPC service was created
    if (!ipc_service_) {
        LOG(ERROR) << "CoreIPCService failed to create.";
        // Handle error appropriately - maybe terminate?
        return;
    }

    // Create the first browser window.
    CefWindowInfo window_info;

    // On Windows we need to specify certain flags and set the parent window handle
    // Embed the browser view into the parent window.
    RECT client_rect;
    CefRect cef_rect;
    if (GetClientRect(parent_hwnd_, &client_rect)) {
        cef_rect.x = client_rect.left;
        cef_rect.y = client_rect.top;
        cef_rect.width = client_rect.right - client_rect.left;
        cef_rect.height = client_rect.bottom - client_rect.top;
        LOG(INFO) << "Setting CEF as child with CefRect: (" << cef_rect.x << "," << cef_rect.y << "," << cef_rect.width << "," << cef_rect.height << ")";
    } else {
        LOG(ERROR) << "Failed to get parent window client rect! Using default 800x600.";
        // Fallback dimensions if GetClientRect fails
        cef_rect.x = 0;
        cef_rect.y = 0;
        cef_rect.width = 800;
        cef_rect.height = 600;
    }
    window_info.SetAsChild(parent_hwnd_, cef_rect);

    // Specify CEF browser settings here.
    CefBrowserSettings browser_settings;

    // Construct the path to the local built UI file
    std::filesystem::path executable_dir = GetExecutableDir_LauncherApp();
    std::filesystem::path index_html_path = executable_dir / "ui" / "dist" / "index.html";

    // Convert the path to a file:/// URL string suitable for CEF
    std::string initial_url = "file:///";
    initial_url += index_html_path.string();

    // Replace backslashes with forward slashes for Windows compatibility
    #ifdef _WIN32
    std::replace(initial_url.begin(), initial_url.end(), '\\', '/');
    #endif

    LOG(INFO) << "Creating browser with start URL: " << initial_url;

    LOG(INFO) << "Attempting CefBrowserHost::CreateBrowser with URL: " << initial_url;

    // Create the browser asynchronously.
    // The CefLifeSpanHandler::OnAfterCreated callback will be invoked later.
    bool browser_created = CefBrowserHost::CreateBrowser(window_info,
                                client_instance_.get(), // Our client handler
                                initial_url,
                                browser_settings,
                                nullptr, // No extra info
                                nullptr); // No request context

    if (!browser_created) {
        LOG(ERROR) << "!!! CefBrowserHost::CreateBrowser call returned false.";
    } else {
        LOG(INFO) << "+++ CefBrowserHost::CreateBrowser call returned true (creation is async).";
    }
}

// Method to set the IPC service after initial construction (usually in the main process)
void LauncherApp::SetIPCService(std::shared_ptr<core::IIPCService> service) {
    LOG(INFO) << "LauncherApp::SetIPCService called.";
    if (ipc_service_) {
        LOG(WARNING) << "SetIPCService called when ipc_service_ was already set.";
    }
    ipc_service_ = std::move(service);
    LOG(INFO) << "IPC Service set in LauncherApp.";
    // Now that we have the IPC service, create the client handler instance
    if (!client_instance_ && ipc_service_) {
        client_instance_ = new LauncherClient(ipc_service_, this); // Pass 'this' (LauncherApp*) 
        LOG(INFO) << "LauncherClient instance created in SetIPCService.";
    } else if (!ipc_service_) {
        LOG(ERROR) << "LauncherApp::SetIPCService - Attempted to set a null IPC service.";
    } else {
        LOG(WARNING) << "LauncherApp::SetIPCService - Client instance already exists.";
    }
}

// Method to get the client instance
CefRefPtr<LauncherClient> LauncherApp::GetLauncherClient() const {
    CEF_REQUIRE_UI_THREAD(); // Restored the macro
    return client_instance_;
}

// CefApp overrides
CefRefPtr<CefBrowserProcessHandler> LauncherApp::GetBrowserProcessHandler() {
    return this;
}

CefRefPtr<CefRenderProcessHandler> LauncherApp::GetRenderProcessHandler() {
    // Return the member variable initialized in the constructor
    return render_process_handler_;
}

void LauncherApp::OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line) {
    // Log the process type and existing command line for debugging
    LOG(INFO) << "OnBeforeCommandLineProcessing - Process Type: " << process_type.ToString()
              << ", Command Line: " << command_line->GetCommandLineString().ToString();

    // Only add the switch for the main browser process (empty process_type)
    if (process_type.empty()) {
        LOG(INFO) << "Processing command line for browser process."; // Updated log message

        // Add the switch to allow file access from file:// URLs
        command_line->AppendSwitch("allow-file-access-from-files");
        LOG(INFO) << "Added switch: --allow-file-access-from-files";

        // Example: Add other switches if needed
        // command_line->AppendSwitch("enable-media-stream");
    }
}

} // namespace game_launcher::cef_integration
