# Catalyst CE

A modern game launcher built with C++, Chromium Embedded Framework (CEF), React, TypeScript, and Tailwind CSS.

## Status

**Under Development:** The core structure is in place, including CEF integration with a React frontend. Current focus is on implementing core functionalities like user authentication and game management.

## Core Technologies

*   **Backend:** C++
*   **UI Framework:** Chromium Embedded Framework (CEF)
*   **Frontend:** React, TypeScript
*   **Styling:** Tailwind CSS
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
├── core/           # Core C++ backend logic (game management, auth - WIP)
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
│   └── ... (standard Vite/React project files)
├── CMakeLists.txt  # Root CMake configuration
└── README.md       # This file
```

## Setup & Building

1.  **Prerequisites:**
    *   CMake
    *   A C++ compiler supporting C++17 (e.g., Visual Studio Build Tools)
    *   Node.js and npm
2.  **Configure CMake:**
    ```bash
    cd path\to\Catalyst-CE
    mkdir build
    cd build
    cmake .. 
    ```
3.  **Build C++ & Frontend:**
    ```bash
    cd path\to\Catalyst-CE\build
    cmake --build . --config Debug  # Or Release
    ```
    *   (The C++ build process automatically triggers the `npm install` and `npm run build` for the UI via a CMake custom command).
4.  **Run:**
    *   The executable will be located in `build\bin\Debug\GameLauncher.exe` (or `Release`).

## Future Goals

*   Implement robust user authentication (`auth_service.h`).
*   Develop game installation, update, and launching capabilities.
*   Refine the UI/UX.
*   Add user settings persistence.
