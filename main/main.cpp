// main/main.cpp
#include "base/cef_logging.h" // Ensure CEF logging is included FIRST
#include "cef_integration/src/logging_macros.h" // Include our central GL_LOG definition

#include "include/cef_app.h"
#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "include/wrapper/cef_helpers.h"
#include "include/cef_browser.h" // For CefBrowser, CefBrowserHost
#include "include/views/cef_window.h" // For CefWindowInfo

#include "game_launcher/cef_integration/launcher_app.h"
#include "game_launcher/core/basic_ipc_service.h" // For BasicIPCService

#include <Windows.h> // Add this include for WinMain types
#include <filesystem> // For path manipulation
#include <string>     // For string manipulation

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
    GL_LOG(INFO) << "Performing cleanup...";
    if (g_hMutex) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
    const wchar_t CLASS_NAME[] = L"GameLauncherWindowClass";
    UnregisterClassW(CLASS_NAME, hInstance); // Unregister the window class
    GL_LOG(INFO) << "Cleanup finished.";
}

#if defined(OS_WIN)
// Use wWinMain for Unicode compatibility, which is preferred by CEF.
int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR    lpCmdLine,
                     int       nCmdShow) {
    // --- Logging Initialization (Moved to absolute start) --- 
    GL_LOG(INFO) << "wWinMain entered.";

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    g_hInstance = hInstance;

    // --- CEF Mandatory First Step --- 
    // Structure for passing command-line arguments to CEF.
    CefMainArgs main_args(hInstance);

    // Create the IPC service instance only in the main browser process.
    game_launcher::core::BasicIPCService ipc_service;

    // Create an instance of our CEF application handler only in the main browser process.
    CefRefPtr<game_launcher::cef_integration::LauncherApp> app(
        new game_launcher::cef_integration::LauncherApp(ipc_service));

    // CEF applications have multiple processes. This executes logic for different process types.
    // Pass the app instance here. If it returns >= 0, it means this is a subprocess and we should exit.
    GL_LOG(INFO) << "Calling CefExecuteProcess...";
    int exit_code = CefExecuteProcess(main_args, app.get(), nullptr /* sandbox_info */);
    GL_LOG(INFO) << "CefExecuteProcess returned: " << exit_code;
    if (exit_code >= 0) {
        GL_LOG(ERROR) << "Exiting as CEF subprocess with code: " << exit_code;
        return exit_code;
    }

    GL_LOG(INFO) << "Continuing as main browser process.";

    // --- Single Instance Check (moved after subprocess check, before window creation) ---+
    g_hMutex = CreateMutex(NULL, TRUE, kMutexName);
    if (g_hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_hMutex) {
            ReleaseMutex(g_hMutex);
            CloseHandle(g_hMutex); // Also close the handle
            g_hMutex = NULL;
        }
        GL_LOG(ERROR) << "GameLauncher already running. Exiting.";
        MessageBox(NULL, L"Another instance of GameLauncher is already running.", L"GameLauncher", MB_OK | MB_ICONWARNING);
        return 1; // Indicate an instance is already running
    }
    GL_LOG(INFO) << "Single instance check passed.";

    GL_LOG(INFO) << "Proceeding with main process initialization.";

    // --- Create the main application window (BEFORE CefInitialize) --- 
    const wchar_t CLASS_NAME[] = L"GameLauncherWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        GL_LOG(FATAL) << "Failed to register window class!";
        Cleanup(hInstance);
        return 1;
    }
    GL_LOG(INFO) << "Window class registered.";

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
        GL_LOG(FATAL) << "Failed to create main window!";
        Cleanup(hInstance);
        return 1;
    }
    GL_LOG(INFO) << "Main window created successfully.";

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
    // settings.remote_debugging_port = 9222; // Temporarily disabled for testing
    // Consider setting log severity, etc.

    // Enable detailed logging to a file (Use absolute path)
    settings.log_severity = LOGSEVERITY_VERBOSE; // Log INFO, WARNING, ERROR, FATAL, VERBOSE
    std::filesystem::path log_path = GetExecutableDir() / L"cef_debug.log";
    CefString(&settings.log_file) = log_path.wstring(); // Correct way to assign wstring

    GL_LOG(INFO) << "Calling CefInitialize... Log file path: " << log_path.string();
    // IMPORTANT: Pass the 'app' instance to CefInitialize for the main process.
    // The sandbox_info is nullptr because we set settings.no_sandbox = true;
    bool cef_initialized = CefInitialize(main_args, settings, app.get(), nullptr);
    if (!cef_initialized) {
        GL_LOG(FATAL) << "CefInitialize failed!";
        Cleanup(hInstance); // Attempt cleanup
        return 1;
    }
    GL_LOG(INFO) << "CefInitialize call completed (Result: " << (cef_initialized ? "Success" : "Failure") << ").";

    // Run the CEF message loop. This function will block until CefQuitMessageLoop()
    // is called.
    GL_LOG(INFO) << "Starting CefRunMessageLoop...";
    CefRunMessageLoop();

    // Shut down CEF. This cleans up CEF resources.
    // Must be called on the main application thread.
    CefShutdown();

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
        if (app) { // Ensure app pointer is valid
            CefRefPtr<game_launcher::cef_integration::LauncherClient> client = app->GetLauncherClient();
            if (client) {
                CefRefPtr<CefBrowser> browser = client->GetBrowser(); // Assumes single browser
                if (browser) {
                    GL_LOG(INFO) << "WM_CLOSE received, requesting browser close (ID: " << browser->GetIdentifier() << ").";
                    browser->GetHost()->CloseBrowser(false); // Request graceful close
                    // Don't call DestroyWindow here. Let OnBeforeClose handle shutdown.
                    return 0; // Indicate message was handled
                } else {
                     GL_LOG(WARNING) << "WM_CLOSE: LauncherClient found, but no browser instance.";
                }
            } else {
                 GL_LOG(WARNING) << "WM_CLOSE: LauncherApp found, but no LauncherClient instance.";
            }
        } else {
             GL_LOG(WARNING) << "WM_CLOSE received, but app pointer is null. Forcing DestroyWindow.";
        }
        // Fallback or if browser closing fails for some reason
        DestroyWindow(hWnd); 
        return 0; // Indicate message was handled

    case WM_DESTROY: 
        // This is called after the window is destroyed. Ensure we quit the message loop.
        GL_LOG(INFO) << "WM_DESTROY received.";
        // CefQuitMessageLoop(); // Should be called by LauncherClient::OnBeforeClose
        PostQuitMessage(0); // Ensure the application truly exits
        if (g_hMutex != NULL) {
            ReleaseMutex(g_hMutex);
            CloseHandle(g_hMutex);
            g_hMutex = NULL;
        }
        return 0; // Return 0 for WM_DESTROY

    case WM_SIZE:
        // Resize the browser window to match the new client area size
        if (app) {
            CefRefPtr<game_launcher::cef_integration::LauncherClient> client = app->GetLauncherClient();
            if (client) {
                 CefRefPtr<CefBrowser> browser = client->GetBrowser();
                 if (browser) {
                     HWND browser_hwnd = browser->GetHost()->GetWindowHandle();
                     if (browser_hwnd) {
                         RECT rect;
                         GetClientRect(hWnd, &rect);
                         MoveWindow(browser_hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
                     }
                 }
            }
        }
        return 0; // Indicate message was handled
    
    case WM_ERASEBKGND:
        // Prevent flickering by not erasing the background (CEF draws over it)
        return 1; // Indicate message was handled

    case WM_SETFOCUS:
        // Pass focus to the browser window
        if (app) {
            CefRefPtr<game_launcher::cef_integration::LauncherClient> client = app->GetLauncherClient();
            if (client) {
                 CefRefPtr<CefBrowser> browser = client->GetBrowser();
                 if (browser) {
                      browser->GetHost()->SetFocus(true);
                 }
            }
        }
        return 0; // Indicate message was handled

    default:
        // Let DefWindowProc handle messages we don't process.
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0; // Dead code path now
}

#else
int main(int argc, char* argv[]) {
    GL_LOG(INFO) << "Game Launcher starting (Non-Windows main - likely unused)...";

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
 
    // Create an instance of our core IPC service
    // In a real app, this might be configured or loaded differently.
    game_launcher::core::BasicIPCService ipc_service;
 
    // Create our CefApp implementation instance.
    // Pass the IPC service to the LauncherApp constructor.
    CefRefPtr<game_launcher::cef_integration::LauncherApp> app(
        new game_launcher::cef_integration::LauncherApp(ipc_service));

    // CEF applications have multiple processes (browser, renderer, etc).
    // This function executes different logic depending on the process type.
    int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
    if (exit_code >= 0) {
        // The sub-process has completed its work.
        GL_LOG(INFO) << "CEF Sub-process exited with code: " << exit_code;
        return exit_code;
    }
    return 0;
}
#endif
