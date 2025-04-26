// main/main.cpp
#include "base/cef_logging.h" // Ensure CEF logging is included FIRST

#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "include/views/cef_window.h" // For CefWindowInfo
#include "include/wrapper/cef_helpers.h"
#include "include/cef_browser.h" // For CefBrowser, CefBrowserHost

#include "game_launcher/cef_integration/launcher_app.h"
#include "game_launcher/core/user_settings.h" // Factory included here
#include "game_launcher/core/authentication_manager.h" // Factory included here
#include "game_launcher/core/game_management_service.h" // Base interface
#include "game_launcher/core/basic_game_management_service.h" // Factory included here
#include "game_launcher/core/background_task_manager.h" // Factory included here
#include "game_launcher/core/json_user_settings.h" // Required for CreateJsonUserSettings
#include "game_launcher/core/core_ipc_service.h" // Required for CreateCoreIPCService
#include "game_launcher/core/auth_status.h" // Required for AuthStatusToString
#include "game_launcher/core/app_settings.h" // Required for AppSettings
#include "game_launcher/core/mock_auth_manager.h" // Include Mock Auth

#include <Windows.h> // Add this include for WinMain types
#include <shlobj.h> // Required for SHGetKnownFolderPath, FOLDERID_RoamingAppData
#include <filesystem> // For path manipulation
#include <string>     // For string manipulation
#include <curl/curl.h> // Added for libcurl global init/cleanup
#include <iostream> // For std::cout debugging
#include <memory> // For std::shared_ptr, std::make_shared, std::unique_ptr

// Forward declare the window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// --- Single Instance Mutex ---+
// Define a unique name for the mutex. Use a GUID to ensure uniqueness.
const LPCWSTR kMutexName = L"{8A71F45E-B2A1-464A-9B9F-1F2E8D7C6B5A}-GameLauncherMutex";
HANDLE g_hMutex = NULL;

// Function to get executable path
std::filesystem::path GetExecutableDir() {
    wchar_t path_buf[MAX_PATH];
    GetModuleFileNameW(NULL, path_buf, MAX_PATH);
    return std::filesystem::path(path_buf).parent_path();
}

// Global handle to the main window (simple approach for now)
// Consider a cleaner approach like passing it through classes if complexity grows.
HWND g_main_hwnd = NULL;

HINSTANCE g_hInstance = NULL; // Global HINSTANCE, useful for window class

void Cleanup(HINSTANCE hInstance) {
    LOG(INFO) << "Performing cleanup...";
    if (g_hMutex) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
    const wchar_t CLASS_NAME[] = L"GameLauncherWindowClass";
    UnregisterClassW(CLASS_NAME, hInstance); // Unregister the window class
    LOG(INFO) << "Cleanup finished.";
}

#if defined(OS_WIN)
// Use wWinMain for Unicode compatibility, which is preferred by CEF.
int wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR    lpCmdLine,
                     int       nCmdShow)
{
    // --- Early CEF Sub-process Handling ---
    // Structure for passing command-line arguments to CEF.
    CefMainArgs main_args(hInstance);

    // Create the CefApp implementation object early. It will be shared across
    // all processes. The IPC service will be set later only in the main process.
    CefRefPtr<game_launcher::cef_integration::LauncherApp> app(new game_launcher::cef_integration::LauncherApp(nullptr));

    // CEF applications have multiple processes. This function executes sub-process
    // logic returns immediately, returning the sub-process exit code.
    // If this is the main process, it returns -1.
    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr);
    if (exit_code >= 0) {
        // The sub-process mode has finished
        LOG(INFO) << "CEF Sub-process exited with code: " << exit_code;
        // Subprocesses don't need further initialization or cleanup here.
        // They might need their own specific cleanup handled via CefApp callbacks.
        return exit_code;
    }
    // --- End of Early CEF Sub-process Handling ---

    // Continue only if this is the main Browser Process (CefExecuteProcess returned -1)
    LOG(INFO) << "Starting Main Browser Process Initialization...";

    // --- Single Instance Check ---
    g_hMutex = CreateMutex(NULL, TRUE, kMutexName);
    if (g_hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_hMutex) {
            ReleaseMutex(g_hMutex);
            CloseHandle(g_hMutex); // Also close the handle
            g_hMutex = NULL;
        }
        LOG(ERROR) << "GameLauncher already running. Exiting.";
        MessageBox(NULL, L"Another instance of GameLauncher is already running.", L"GameLauncher", MB_OK | MB_ICONWARNING);
        return 1; // Indicate an instance is already running
    }
    LOG(INFO) << "Single instance check passed.";

    // --- Initialize libcurl globally ---
    CURLcode curl_res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curl_res != CURLE_OK) {
        LOG(FATAL) << "curl_global_init() failed: " << curl_easy_strerror(curl_res);
        Cleanup(hInstance); // Attempt cleanup before exiting
        return 1;
    }
    LOG(INFO) << "libcurl initialized globally.";

    // ------------------------------------------------------------------------
    // Create Core Services (using factories)
    // ------------------------------------------------------------------------
    namespace fs = std::filesystem;

    // Helper function to get the Roaming AppData path
    auto GetAppDataPath = []() -> std::optional<std::filesystem::path> {
        wchar_t* path_tmp = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path_tmp))) {
            std::filesystem::path appdata_path(path_tmp);
            CoTaskMemFree(path_tmp); // Free the allocated memory
            LOG(INFO) << "Found AppData path: " << appdata_path.string();
            return appdata_path;
        }
        LOG(ERROR) << "Failed to get Roaming AppData path.";
        return std::nullopt;
    };

    fs::path settings_dir;
    auto appdata_path_opt = GetAppDataPath();
    if (!appdata_path_opt) {
        LOG(ERROR) << "Failed to get AppData path. Using executable directory as fallback for settings.";
        settings_dir = GetExecutableDir(); // Fallback, not ideal
    } else {
         settings_dir = *appdata_path_opt / "WindsurfLauncher"; // App-specific folder
    }

    fs::path settings_file_path = settings_dir / "settings.json";

    // Create parent directory if it doesn't exist
    try {
        if (!fs::exists(settings_dir)) {
            fs::create_directories(settings_dir);
            LOG(INFO) << "Created settings directory: " << settings_dir.string();
        }
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to create settings directory '" << settings_dir.string() << "': " << e.what();
        // Continue for now, settings might not save/load correctly
    }

    LOG(INFO) << "Using settings file path: " << settings_file_path.string();

    // Create background task manager
    auto background_task_manager = game_launcher::core::CreateBackgroundTaskManager();

    // Load user settings
    auto user_settings = std::make_shared<game_launcher::core::JsonUserSettings>(settings_file_path);
    if (!user_settings) {
        LOG(ERROR) << "Failed to create UserSettings.";
        curl_global_cleanup();
        Cleanup(hInstance);
        return 1;
    }

    // Create Mock Auth Manager (replace with real one later)
    LOG(INFO) << "Creating MockAuthManager...";
    auto mock_auth_manager = game_launcher::core::CreateMockAuthManager();
    LOG(INFO) << "MockAuthManager created.";

    // Create game management service
    fs::path exe_dir = GetExecutableDir();
    fs::path games_json_path = exe_dir / "resources" / "games.json";
    LOG(INFO) << "Using games definition file path: " << games_json_path.string();

    auto game_manager = game_launcher::core::CreateBasicGameManager(
        games_json_path,
        user_settings,
        background_task_manager.get() // Pass the background task manager
    );
    if (!game_manager) {
        LOG(ERROR) << "Failed to create GameManager service.";
        curl_global_cleanup();
        Cleanup(hInstance);
        return 1;
    }

    // Declare the main service pointer first (must be outside try block)
    std::shared_ptr<game_launcher::core::IIPCService> ipc_service;

    try {
        LOG(INFO) << "Creating CoreIPCService into temporary unique_ptr...";
        auto temp_ipc_service = game_launcher::core::CoreIPCService::CreateCoreIPCService(
            game_manager,           // Pass the created game_manager
            std::move(mock_auth_manager), // Move ownership of unique_ptr
            user_settings,          // Pass the created user_settings
            background_task_manager.get() // Pass the raw pointer as needed
        );
        LOG(INFO) << "Temporary CoreIPCService unique_ptr created.";

        if (!temp_ipc_service) {
            LOG(FATAL) << "CoreIPCService::CreateCoreIPCService returned nullptr unexpectedly.";
            MessageBox(NULL, L"Core service creation returned null. Exiting.", L"Launcher Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        // Move from temporary to the final variable
        LOG(INFO) << "Moving temporary unique_ptr to ipc_service...";
        ipc_service = std::shared_ptr<game_launcher::core::IIPCService>(std::move(temp_ipc_service)); // Explicit construction
        LOG(INFO) << "Moved temporary unique_ptr to ipc_service.";

    } catch (const std::exception& e) {
        LOG(FATAL) << "Caught std::exception during CoreIPCService creation/assignment: " << e.what();
        MessageBoxA(NULL, (std::string("Exception during core service init: ") + e.what()).c_str(), "Launcher Error", MB_OK | MB_ICONERROR);
        return 1;
    } catch (...) {
        LOG(FATAL) << "Caught unknown exception during CoreIPCService creation/assignment.";
        MessageBox(NULL, L"Unknown exception during core service init. Exiting.", L"Launcher Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Set the created IPC service in the LauncherApp instance
    LOG(INFO) << "Setting IPC service in LauncherApp...";
    app->SetIPCService(ipc_service);

    LOG(INFO) << "Proceeding with main window initialization.";

    // --- Create the main application window (BEFORE CefInitialize) --- 
    const wchar_t CLASS_NAME[] = L"GameLauncherWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        LOG(FATAL) << "Failed to register window class!";
        curl_global_cleanup(); // Cleanup curl
        Cleanup(hInstance);
        return 1;
    }
    LOG(INFO) << "Window class registered.";

    // Create the window.
    g_main_hwnd = CreateWindowExW(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Windsurf Game Launcher",      // Window text
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, // Window style (clip children needed for CEF)

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (g_main_hwnd == NULL) {
        LOG(FATAL) << "Failed to create main window!";
        curl_global_cleanup(); // Cleanup curl
        Cleanup(hInstance);
        return 1;
    }
    LOG(INFO) << "Main window created successfully.";

    // Set the parent HWND in the LauncherApp instance *before* CefInitialize
    app->SetParentHWND(g_main_hwnd);

    UpdateWindow(g_main_hwnd);

    // --- Store app pointer for WndProc ---+
    // Store the LauncherApp pointer in the window's user data so WndProc can access it.
    SetWindowLongPtr(g_main_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app.get()));

    ShowWindow(g_main_hwnd, nCmdShow);

    // --- CEF Initialization --- 
    CefSettings settings;
    settings.no_sandbox = true; // Disable sandbox for simplicity during dev
    settings.remote_debugging_port = 8088; // Enable remote debugging

    // Enable detailed logging to a file (Use absolute path)
    settings.log_severity = LOGSEVERITY_VERBOSE; // Log INFO, WARNING, ERROR, FATAL, VERBOSE
    std::filesystem::path log_path = GetExecutableDir() / L"cef_debug.log";
    CefString(&settings.log_file) = log_path.wstring(); // Correct way to assign wstring

    // Explicitly set the path to the subprocess executable
    std::filesystem::path exe_path = GetExecutableDir() / L"GameLauncher.exe"; // Adjust if your exe name is different
    CefString(&settings.browser_subprocess_path) = exe_path.wstring();

    LOG(INFO) << "Attempting CefInitialize. Log file configured at: " << log_path.string();

    // IMPORTANT: Pass the 'app' instance to CefInitialize for the main process.
    // The sandbox_info is nullptr because we set settings.no_sandbox = true;
    bool cef_initialized = CefInitialize(main_args, settings, app.get(), nullptr);

    LOG(INFO) << "CefInitialize returned: " << (cef_initialized ? "true" : "false");

    if (!cef_initialized) {
        LOG(FATAL) << "CefInitialize failed! Check previous logs for details.";
        curl_global_cleanup(); // Cleanup curl
        Cleanup(hInstance); // Attempt cleanup
        return 1;
    }
    LOG(INFO) << "CefInitialize call completed (Result: " << (cef_initialized ? "Success" : "Failure") << ").";

    // Run the CEF message loop. This function will block until CefQuitMessageLoop()
    // is called.
    LOG(INFO) << "Starting CefRunMessageLoop...";
    CefRunMessageLoop();

    // Shut down CEF. This cleans up CEF resources.
    // Must be called on the main application thread.
    LOG(INFO) << "Shutting down CEF...";
    CefShutdown();
    LOG(INFO) << "CEF shutdown complete.";

    // --- Cleanup libcurl globally ---
    LOG(INFO) << "Cleaning up libcurl...";
    curl_global_cleanup();
    LOG(INFO) << "libcurl cleanup complete.";

    // Release the mutex if this instance is closing
    if (g_hMutex != NULL) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }

    return 0; // Indicate success
}

// Basic Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // Retrieve the LauncherApp pointer stored in the window's user data.
    game_launcher::cef_integration::LauncherApp* app = reinterpret_cast<game_launcher::cef_integration::LauncherApp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message) {
    case WM_CLOSE:
        // Attempt to close the browser window gracefully first
        if (app && !app->IsShuttingDown()) { // Ensure app pointer is valid and not shutting down
            CefRefPtr<game_launcher::cef_integration::LauncherClient> client = app->GetLauncherClient();
            if (client) {
                CefRefPtr<CefBrowser> browser = client->GetBrowser(); // Assumes single browser
                if (browser) {
                    LOG(INFO) << "WM_CLOSE received, requesting browser close (ID: " << browser->GetIdentifier() << ").";
                    browser->GetHost()->CloseBrowser(false); // Request graceful close
                    // Don't call DestroyWindow here. Let OnBeforeClose handle shutdown.
                    return 0; // Indicate message was handled
                } else {
                     LOG(WARNING) << "WM_CLOSE: LauncherClient found, but no browser instance.";
                }
            } else {
                 LOG(WARNING) << "WM_CLOSE: LauncherApp found, but no LauncherClient instance.";
            }
        } else {
             LOG(WARNING) << "WM_CLOSE received, but app pointer is null or shutting down. Forcing DestroyWindow.";
        }
        // Fallback or if browser closing fails for some reason
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        // Post a quit message to the application's message loop
        PostQuitMessage(0);
        break;

    default:
        // Handle any messages we didn't process
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#else
int main(int argc, char* argv[]) {
    LOG(INFO) << "Game Launcher starting (Non-Windows main - likely unused)...";

    // --- CEF Initialization --- 
    // Structure for passing command-line arguments.
    CefMainArgs main_args(argc, argv);

    // Optional sandbox information
    // For the initial setup, we can disable the sandbox for simplicity.
    // Later, we'll need to configure it properly (Phase 5a).
    void* sandbox_info = nullptr;
 
    // Provide CEF with command-line arguments.
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);
 
    // Create our CefApp implementation instance.
    CefRefPtr<game_launcher::cef_integration::LauncherApp> app(
        new game_launcher::cef_integration::LauncherApp());

    // Initialize libcurl globally
    CURLcode curl_res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (curl_res != CURLE_OK) {
        // Handle error appropriately - log or print
        fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(curl_res));
        return 1;
    }

    // CEF applications have multiple processes (browser, renderer, etc).
    // This function executes different logic depending on the process type.
    int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
    if (exit_code >= 0) {
        // The sub-process has completed its work.
        LOG(INFO) << "CEF Sub-process exited with code: " << exit_code;
        curl_global_cleanup(); // Cleanup curl
        return exit_code;
    }

    // Initialize CEF.
    CefSettings settings;
    settings.no_sandbox = true; // Disable sandbox for simplicity during dev
    settings.remote_debugging_port = 8088; // Enable remote debugging

    // Enable detailed logging to a file (Use absolute path)
    settings.log_severity = LOGSEVERITY_VERBOSE; // Log INFO, WARNING, ERROR, FATAL, VERBOSE
    std::filesystem::path log_path = std::filesystem::current_path() / "cef_debug.log";
    CefString(&settings.log_file) = log_path.wstring(); // Correct way to assign wstring

    // Explicitly set the path to the subprocess executable
    std::filesystem::path exe_path = std::filesystem::current_path() / "GameLauncher"; // Adjust if your exe name is different
    CefString(&settings.browser_subprocess_path) = exe_path.wstring();

    LOG(INFO) << "Attempting CefInitialize. Log file configured at: " << log_path.string();

    // IMPORTANT: Pass the 'app' instance to CefInitialize for the main process.
    // The sandbox_info is nullptr because we set settings.no_sandbox = true;
    bool cef_initialized = CefInitialize(main_args, settings, app.get(), nullptr);

    LOG(INFO) << "CefInitialize returned: " << (cef_initialized ? "true" : "false");

    if (!cef_initialized) {
        LOG(FATAL) << "CefInitialize failed! Check previous logs for details.";
        curl_global_cleanup(); // Cleanup curl
        return 1;
    }
    LOG(INFO) << "CefInitialize call completed (Result: " << (cef_initialized ? "Success" : "Failure") << ").";

    // Shut down CEF.
    CefShutdown();

    // Cleanup libcurl globally
    curl_global_cleanup();

    return 0;
}
#endif
