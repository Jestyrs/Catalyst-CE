# Catalyst CE

A modern game launcher built with C++, Chromium Embedded Framework (CEF), React, TypeScript, and Tailwind CSS.

## Status

**Under Development:** The core structure is in place, including CEF integration with a React frontend. User authentication via Auth0 (Username/Password and Google Sign-In) is implemented. Focus is shifting towards game management and other core launcher features.

## Core Technologies

*   **Backend:** C++
*   **UI Framework:** Chromium Embedded Framework (CEF)
*   **Frontend:** React, TypeScript
*   **Styling:** Tailwind CSS
*   **Authentication:** Auth0
*   **Build Systems:** CMake (C++), Vite (Frontend)
*   **IPC:** CEF Process Messaging

## Project Structure

```
.
├── .dependencies/  # External dependencies (managed by CMake FetchContent)
├── build/          # CMake build output
├── cef_integration/ # C++ code for CEF browser setup, IPC, API handlers
│   ├── include/
│   └── src/
├── core/           # Core C++ backend logic (game management, auth)
│   ├── include/
│   └── src/
├── main/           # Main C++ application entry point (Win32 window setup)
│   └── src/
├── platform/       # Platform-specific C++ code
│   ├── include/
│   └── src/
├── ui/             # React/TypeScript frontend source code
│   ├── public/
│   ├── src/
│   ├── .env.example    # Example environment variables for Auth0
│   └── ... (standard Vite/React project files)
├── CMakeLists.txt  # Root CMake configuration
└── README.md       # This file
```

## Setup

### Prerequisites

*   **Git:** For cloning the repository.
*   **CMake:** (Version 3.20 or higher recommended).
*   **C++ Compiler:** Supporting C++20 (e.g., Visual Studio 2022 with Desktop development workload).
*   **Node.js & npm:** (Latest LTS recommended) For managing frontend dependencies and running the development server.
*   **Auth0 Account:** Required for authentication features.
    *   Create a **Single Page Application (SPA)** in your Auth0 dashboard.
    *   Note your **Domain** and **Client ID**.
    *   In the Application Settings, add `http://localhost:5173` to the **Allowed Callback URLs**. This is crucial for the development workflow.
    *   *(Optional but Recommended)*: Copy `.env.example` in the `ui` directory to `.env` and fill in your Auth0 Domain and Client ID. The React app reads these (currently hardcoded for simplicity, but `.env` is better practice).

### Building for Production/Distribution

This builds the C++ application and bundles the latest frontend code directly into the executable's resources.

1.  **Clone:** `git clone <repository_url>`
2.  **Configure CMake:**
    ```bash
    cd Catalyst-CE
    mkdir build
    cd build
    cmake ..
    ```
3.  **Build:**
    ```bash
    # From the 'build' directory
    cmake --build . --config Release # Or Debug
    ```
    *   This command compiles the C++ code and **automatically** runs `npm install` and `npm run build` in the `ui` directory, copying the built frontend assets into the C++ build output.
4.  **Run:**
    *   The executable will be in `build\bin\Release\GameLauncher.exe` (or `Debug`). The application will load the bundled UI using `file:///` paths.

### Development Workflow

This workflow uses the Vite development server for hot module replacement (HMR) during frontend development.

1.  **Ensure Prerequisites are met.** (Including Auth0 setup with the correct Callback URL).
2.  **Start Vite Dev Server:**
    ```bash
    cd path/to/Catalyst-CE/ui
    npm install # If you haven't already
    npm run dev
    ```
    *   This server will host the UI at `http://localhost:5173`.
3.  **Configure & Build C++ (if needed):**
    ```bash
    # If you haven't configured CMake yet:
    cd path/to/Catalyst-CE
    mkdir build
    cd build
    cmake ..

    # Build the C++ application (Debug configuration is typical for development)
    # From the 'build' directory
    cmake --build . --config Debug
    ```
    *   The C++ build process still runs the `npm build` steps, but they aren't strictly necessary for *this* workflow since we load from Vite.
4.  **Run C++ Application:**
    ```bash
    # From the 'build' directory
    .\bin\Debug\GameLauncher.exe
    ```
    *   The C++ application (specifically `cef_integration/src/launcher_app.cpp`) is currently configured to load the UI from `http://localhost:5173` when built. Changes made in the `ui` source code should reflect near-instantly in the running application thanks to Vite HMR.

## Authentication

*   Uses Auth0 for handling user login and sessions.
*   Supports Username/Password database connections and Google Sign-In.
*   Frontend uses `@auth0/auth0-react` SDK.
*   Backend (`CoreIPCService`) interacts with Auth0 (future enhancement: validate tokens).

## Future Goals

*   Develop robust game installation, update, and launching capabilities.
*   Refine the UI/UX further.
*   Implement persistent user settings storage.
*   Secure backend API calls using Auth0 access tokens.
*   Add CI/CD pipeline.
