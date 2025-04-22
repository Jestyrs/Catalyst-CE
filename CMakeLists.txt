# CMakeLists.txt (Root)

cmake_minimum_required(VERSION 3.22) # Specify minimum CMake version

project(GameLauncher VERSION 0.1.0 LANGUAGES CXX)

# Set the MSVC runtime library globally for consistency (/MDd or /MD)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC Runtime Library" FORCE)

# --- FetchContent Configuration ---
# Store downloaded sources outside the main build dir for persistence
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/.dependencies)
message(STATUS "FetchContent base directory set to: ${FETCHCONTENT_BASE_DIR}")
include(FetchContent)
# --- End FetchContent Config ---

# Set C++ Standard FIRST
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer explicit standard features

# Define NOMINMAX globally to prevent Windows header conflicts (min/max macros)
add_definitions(-DNOMINMAX)

# --- MSVC Runtime Library Enforcement ---
if(MSVC)
    # Ensure our project uses the same flags as CEF for things like exceptions/RTTI if needed
    # add_compile_options(/EHsc) # Example: if CEF needs exceptions

    # CMP0091: MSVC runtime library flags are selected by an abstraction.
    cmake_policy(SET CMP0091 NEW)
    # Use the Debug/Release DLL runtime (/MDd, /MD). Change if you require static runtime (/MTd, /MT).
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    message(STATUS "MSVC Runtime Library set to: ${CMAKE_MSVC_RUNTIME_LIBRARY}") # For verification

    # Force iterator debugging off for Debug builds globally to match pre-built CEF
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " /D_ITERATOR_DEBUG_LEVEL=0 /D_HAS_ITERATOR_DEBUGGING=0")
    message(STATUS "Globally forcing _ITERATOR_DEBUG_LEVEL=0 for Debug. CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")

endif()
# --- End of block ---

# --- Fetch Dependencies ---
# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.12.1
)

# Abseil (for Status, StatusOr, etc.)
# Set Abseil options BEFORE declaring it
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "Propagate C++ standard to Abseil")
FetchContent_Declare(
    abseil-cpp
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG        20230802.1 # Use a specific LTS tag or commit for stability
)

# nlohmann/json (for JSON parsing)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3 # Use a specific stable tag
)

FetchContent_MakeAvailable(abseil-cpp)

# --- Make GTest Available AFTER Abseil --- 
# (In case GTest has potential dependency or configuration interactions)
FetchContent_MakeAvailable(googletest)

# --- Make nlohmann/json Available --- 
FetchContent_MakeAvailable(nlohmann_json)

# Enable testing with CTest
enable_testing()
include(CTest)

# --- CEF Dependency --- 
# Define CEF version and platform
set(CEF_VERSION "135.0.21+gd008a99+chromium-135.0.7049.96") # Example version
set(CEF_PLATFORM "windows64")

# Override the default CEF runtime library setting to match our project (/MD or /MDd)
set(CEF_RUNTIME_LIBRARY_FLAG "$<$<CONFIG:Debug>:/MDd;/MD>" CACHE STRING "Specify CEF runtime library flag (Debug: /MDd, Release: /MD)" FORCE)

FetchContent_Declare(
    cef_binary
    URL "https://cef-builds.spotifycdn.com/cef_binary_${CEF_VERSION}_${CEF_PLATFORM}.tar.bz2"
    URL_HASH SHA256=af85614db3460aa497acf2124581ede227a013c00c62b61ccb2367644230e4e6 # HASH FOR VERSION 135.0.21 windows64
    CMAKE_ARGS
        # Use CEF's intended mechanism for runtime flags
        "-DCEF_RUNTIME_LIBRARY_FLAG=${CEF_RUNTIME_LIBRARY_FLAG}"
        # Override CEF's internal debug definitions list to add iterator debugging
        "-DCEF_COMPILER_DEFINES_DEBUG=_DEBUG;_ITERATOR_DEBUG_LEVEL=2"
)

# Explicitly tell FetchContent that the source directory is already populated (manual download)
# and it should skip download/update/patch steps, using the existing files.
set(FETCHCONTENT_FULLY_POPULATED TRUE)

# --- Make CEF available via FetchContent ---
# This will download/unpack CEF if needed and process its top-level CMakeLists.txt.
FetchContent_MakeAvailable(cef_binary)

# --- Explicitly find the CEF package within the fetched directory ---
# Set CEF_ROOT, which FindCEF.cmake uses to locate the distribution.
set(CEF_ROOT ${cef_binary_SOURCE_DIR} CACHE PATH "Path to CEF distribution root")
# Add the CEF cmake module directory to the module search path
list(APPEND CMAKE_MODULE_PATH "${cef_binary_SOURCE_DIR}/cmake")

# --- Configure CEF ---
# REMOVED: Explicit setting of CEF_RUNTIME_LIBRARY_FLAG.
# CMAKE_MSVC_RUNTIME_LIBRARY (set earlier) and CMP0091 (set earlier)
# should handle the runtime library setting correctly for all targets,
# including those brought in by CEF.

# REMOVED: find_package(CEF REQUIRED MODULE). Relying on targets from FetchContent_MakeAvailable.

# --- Build libcef_dll_wrapper from source ---
# REMOVED: Explicit add_subdirectory call. FetchContent_MakeAvailable already handles building it.

# Ensure libcef_dll_wrapper uses the same runtime as the main project
# Setting CMAKE_MSVC_RUNTIME_LIBRARY before find_package should handle this via CMP0091.

# --- Project Structure ---
# Add subdirectories containing their own CMakeLists.txt files
add_subdirectory(platform)
add_subdirectory(core)
add_subdirectory(cef_integration)

# === Executable Target: GameLauncher ===
# Define the executable target name first.
# Sources will be added in the main subdirectory.
add_executable(GameLauncher WIN32
    main/main.cpp  # Correct path to the main entry point
    # Assuming other components (app, ipc) are built as libraries
    # in subdirectories and linked via target_link_libraries below.
)

# Copy the UI directory to the output directory after build
add_custom_command(
    TARGET GameLauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_SOURCE_DIR}/ui"
        "$<TARGET_FILE_DIR:GameLauncher>/ui"
    COMMENT "Copying UI resources"
    VERBATIM
)

# Define necessary preprocessor symbols for Windows Unicode support
target_compile_definitions(GameLauncher PRIVATE UNICODE _UNICODE)

# --- Link Libraries ---
# Link against common libraries for all platforms
target_link_libraries(GameLauncher PRIVATE
    cef_integration             # Our CEF integration layer
    core                        # Core logic (includes IIPCService impl)
    libcef_dll_wrapper        # Use target built by FetchContent_MakeAvailable
    libcef                    # Use target built by FetchContent_MakeAvailable
    # absl::base  # REMOVED: Let 'core' link its specific Abseil dependencies
)

target_include_directories(GameLauncher PUBLIC
    ${PROJECT_SOURCE_DIR}      # Add project root for includes like "cef_integration/..."
    ${cef_binary_SOURCE_DIR}  # Root directory containing cef_app.h, etc.
)

target_link_directories(GameLauncher PRIVATE
    "${cef_binary_SOURCE_DIR}/Release"  # Directory containing libcef.lib
    # Add Debug directory if needed for Debug builds, though Release is common
)

# Windows specific settings
if(WIN32)
    target_link_libraries(GameLauncher PRIVATE
        User32.lib  # Often needed for basic windowing/message loop
        Shell32.lib # May be needed depending on CEF features used
        Ole32.lib
        Gdi32.lib
        Advapi32.lib
        Shlwapi.lib
    )
    # Set the output directory for convenience during development
    set_target_properties(GameLauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    # Copy CEF resources to the output directory
    # Adjust paths based on your CEF binary distribution structure
    set(CEF_RESOURCES_DIR "${PROJECT_SOURCE_DIR}/.dependencies/cef_binary-src/Release") # Adjust if using Debug CEF build
    set(CEF_RESOURCES_DATA_DIR "${PROJECT_SOURCE_DIR}/.dependencies/cef_binary-src/Resources")
    set(CEF_LOCALES_DIR "${PROJECT_SOURCE_DIR}/.dependencies/cef_binary-src/Resources/locales")

    # Resources needed for basic operation
    set(CEF_ESSENTIAL_RESOURCES
        "${cef_binary_SOURCE_DIR}/Release/chrome_elf.dll"
        "${cef_binary_SOURCE_DIR}/Release/d3dcompiler_47.dll"
        "${cef_binary_SOURCE_DIR}/Release/libcef.dll"
        "${cef_binary_SOURCE_DIR}/Release/libEGL.dll"
        "${cef_binary_SOURCE_DIR}/Release/libGLESv2.dll"
        "${cef_binary_SOURCE_DIR}/Release/vk_swiftshader.dll"
        "${cef_binary_SOURCE_DIR}/Release/vk_swiftshader_icd.json"
        "${cef_binary_SOURCE_DIR}/Release/vulkan-1.dll"
        "${cef_binary_SOURCE_DIR}/Release/v8_context_snapshot.bin"
        "${cef_binary_SOURCE_DIR}/Resources/chrome_100_percent.pak"
        "${cef_binary_SOURCE_DIR}/Resources/chrome_200_percent.pak"
        "${cef_binary_SOURCE_DIR}/Resources/icudtl.dat"
        "${cef_binary_SOURCE_DIR}/Resources/resources.pak"
    )

    # Add a post-build command to copy resources
    add_custom_command(
        TARGET GameLauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CEF_ESSENTIAL_RESOURCES} # Pass the full list
            "$<TARGET_FILE_DIR:GameLauncher>" # Ensure destination is quoted
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${cef_binary_SOURCE_DIR}/Resources/locales"
            "$<TARGET_FILE_DIR:GameLauncher>/locales" # Ensure destination is quoted
        COMMENT "Copying CEF resources"
        VERBATIM
    )
endif()

# === Installation (Optional) ===
# install(TARGETS GameLauncher DESTINATION bin)

# --- Testing Configuration (Placeholder) ---
# TODO: Enable testing and add tests discovered from the 'tests' subdirectory
# add_test(NAME CoreUnitTests COMMAND ...)

# --- Installation Rules (Placeholder) ---
# TODO: Define installation rules for the executable, libraries, CEF resources, and UI assets
# install(TARGETS GameLauncherLauncher DESTINATION bin)
# install(FILES ${CEF_RESOURCES} DESTINATION bin)
# install(DIRECTORY ui/ DESTINATION share/GameLauncher/ui)

message(STATUS "Project: ${PROJECT_NAME} configured.")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")
