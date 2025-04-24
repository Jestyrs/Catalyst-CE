# CMakeLists.txt (Root)

cmake_minimum_required(VERSION 3.20) # Decreased version for compatibility

project(GameLauncher VERSION 0.1.0 LANGUAGES CXX)

# --- MSVC Runtime Library --- #
if(MSVC)
  # Force use of Dynamic Runtime Library (MDd for Debug, MD for Release)
  # This prevents LNK2038 errors when linking different libs/executables
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  message(STATUS "MSVC Runtime Library forced to: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()

# Set the C++ standard to C++20 (or C++17 if needed)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define NOMINMAX globally to prevent Windows header conflicts (min/max macros)
add_definitions(-DNOMINMAX)

# --- Add Iterator Debug Flags for MSVC Debug --- #
if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_ITERATOR_DEBUG_LEVEL=0 /D_HAS_ITERATOR_DEBUGGING=0")
  message(STATUS "Appended iterator debug flags to CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

# --- Disable Building Tests from Dependencies ---
set(BUILD_TESTING OFF CACHE BOOL "Disable building tests from dependencies" FORCE)

# --- FetchContent Configuration ---
# Store downloaded sources outside the main build dir for persistence
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/.dependencies)
message(STATUS "FetchContent base directory set to: ${FETCHCONTENT_BASE_DIR}")
set(FETCHCONTENT_QUIET OFF) # Add this line for verbose output
include(FetchContent)

# --- Explicitly configure libcurl features EARLIER ---
# These will be used when add_subdirectory(curl) is called later
# --- Configure libcurl build options BEFORE MakeAvailable ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build curl static lib" FORCE) # Prefer static lib
set(BUILD_STATIC_LIBS ON CACHE BOOL "Build curl static lib" FORCE) # Explicitly ON
set(BUILD_CURL_EXE OFF CACHE BOOL "Disable curl executable tool" FORCE)
set(CMAKE_USE_SCHANNEL ON CACHE BOOL "Use Schannel on Windows for curl" FORCE) # Often preferred on Win
set(CURL_USE_OPENSSL OFF CACHE BOOL "Disable OpenSSL for curl" FORCE) # Use Schannel instead
set(HTTP_ONLY ON CACHE BOOL "Build curl with only HTTP(S) support" FORCE)
set(ENABLE_MANUAL OFF CACHE BOOL "Disable curl manual pages" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "Disable Public Suffix List support" FORCE)
set(ENABLE_LDAP OFF CACHE BOOL "Disable LDAP support" FORCE)
set(ENABLE_LDAPS OFF CACHE BOOL "Disable LDAPS support" FORCE)
# --- End early libcurl configuration ---
# --- End libcurl configuration ---

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

# picosha2 (for SHA256 hashing - header only)
FetchContent_Declare(
    picosha2
    GIT_REPOSITORY https://github.com/okdshin/picosha2.git
    GIT_TAG        v1.0.0 # Use tag suggested by user
)

# libcurl (Manual Download) - TEMPORARY WORKAROUND
FetchContent_Declare(
  curl
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG        curl-8_13_0 # Use the correct tag format
  OVERRIDE_FIND_PACKAGE     # Recommended for consistent find_package behavior
)

FetchContent_MakeAvailable(curl)       # Builds curl with options set above
find_package(CURL REQUIRED)          # Ensure targets like curl::libcurl are imported

FetchContent_MakeAvailable(abseil-cpp)
FetchContent_MakeAvailable(picosha2)   # Download/populate picosha2

# --- Make GTest Available AFTER Abseil --- 
# (In case GTest has potential dependency or configuration interactions)
FetchContent_MakeAvailable(googletest)

# --- Make nlohmann/json Available --- 
FetchContent_MakeAvailable(nlohmann_json)

# Define CEF Version and Platform FIRST
set(CEF_VERSION "135.0.21+gd008a99+chromium-135.0.7049.96" CACHE STRING "CEF Version to download") # Example version
set(CEF_PLATFORM "windows64" CACHE STRING "CEF Platform (e.g., windows64, linux64, macos64, macosarm64)")
message(STATUS "Using CEF Version: ${CEF_VERSION} for Platform: ${CEF_PLATFORM}")

# Chromium Embedded Framework (CEF)
FetchContent_Declare(
    cef_binary
    URL "https://cef-builds.spotifycdn.com/cef_binary_${CEF_VERSION}_${CEF_PLATFORM}.tar.bz2"
    # Example HASH for 118.6.6+g138057a+chromium-118.0.5993.90 windows64
    # URL_HASH SHA256=YOUR_SHA256_HASH_HERE # <-- IMPORTANT: Add the correct SHA256 hash for the chosen version/platform
    # Example HASH for 135.0.21+gd008a99+chromium-135.0.7049.96 windows64
    URL_HASH SHA256=af85614db3460aa497acf2124581ede227a013c00c62b61ccb2367644230e4e6
)

# Set CEF flags BEFORE making it available
# Force CEF to build libcef_dll_wrapper with the dynamic runtime (/MD or /MDd)
# Use generator expression to handle Debug (/MDd) and Release (/MD) automatically
set(CEF_RUNTIME_LIBRARY_FLAG "/MD$<$<CONFIG:Debug>:d>" CACHE STRING "Force CEF wrapper runtime library (/MDd or /MD)" FORCE)
message(STATUS "Setting CEF_RUNTIME_LIBRARY_FLAG to ${CEF_RUNTIME_LIBRARY_FLAG} (evaluated: /MD$<$<CONFIG:Debug>:d>)")

FetchContent_MakeAvailable(cef_binary)

# Explicitly exclude CEF tests/example targets to avoid runtime library conflicts
set_target_properties(ceftests PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(cefsimple PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Disable the sandbox BEFORE making CEF available
set(USE_SANDBOX OFF CACHE BOOL "Disable CEF sandbox" FORCE)

# Ensure libcef_dll_wrapper uses the same runtime as the main project
# Setting CMAKE_MSVC_RUNTIME_LIBRARY before find_package should handle this via CMP0091.

# === Find Node/NPM ===
find_program(NODEJS_EXECUTABLE NAMES node nodejs)
find_program(NPM_EXECUTABLE NAMES npm)

if(NOT NODEJS_EXECUTABLE OR NOT NPM_EXECUTABLE)
    message(FATAL_ERROR "NodeJS and/or NPM not found. Please install NodeJS (which includes NPM) and ensure it's in your PATH.")
else()
    message(STATUS "Found NodeJS: ${NODEJS_EXECUTABLE}")
    message(STATUS "Found NPM: ${NPM_EXECUTABLE}")
endif()

# === Custom Target for Building UI ===
if(NPM_EXECUTABLE)
    add_custom_target(BuildUI ALL
        COMMAND ${NPM_EXECUTABLE} install  # Run npm install first
        COMMAND ${NPM_EXECUTABLE} run build # Then run the build script
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ui
        COMMENT "Building UI assets (npm install && npm run build)"
        VERBATIM
    )
else()
    # Create a dummy target if npm isn't found, build will likely fail later but avoids CMake error here
    add_custom_target(BuildUI ALL
        COMMENT "Skipping UI build: NPM not found."
    )
endif()

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

# Ensure GameLauncher depends on the UI build completing first
add_dependencies(GameLauncher BuildUI)

# Custom command to copy BUILT UI resources (dist folder)
# Ensure the target directory exists before copying
add_custom_command(
    TARGET GameLauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:GameLauncher>/ui/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_SOURCE_DIR}/ui/dist" # <<< Copy the BUILT 'dist' directory
        "$<TARGET_FILE_DIR:GameLauncher>/ui/dist" # <<< To the 'dist' subdirectory in the target
    COMMENT "Copying built UI resources (ui/dist)"
    VERBATIM
)

# Custom command to copy resource files (manifests, etc.)
add_custom_command(
    TARGET GameLauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:GameLauncher>/resources" # Create destination first
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:GameLauncher>/resources"
    COMMENT "Copying resource files"
    VERBATIM
)

# --- CEF Resource Copying (specific files/dirs) ---
# CEF requires specific resources alongside the executable.
# Define necessary preprocessor symbols for Windows Unicode support
target_compile_definitions(GameLauncher PRIVATE UNICODE _UNICODE)

# --- Link Libraries ---
# Link against common libraries for all platforms
target_link_libraries(GameLauncher PRIVATE
    cef_integration             # Our CEF integration layer (which links core PUBLICLY)
    libcef_dll_wrapper        # Use target built by FetchContent_MakeAvailable
    libcef                    # Use target built by FetchContent_MakeAvailable
    nlohmann_json::nlohmann_json # Link nlohmann json for headers included by GameLauncher
    CURL::libcurl             # Link curl for headers included by GameLauncher (Corrected case)
    absl::status              # Explicit link for core's public interface dependency
    absl::statusor            # Explicit link for core's public interface dependency
    # Windows specific libraries
    Shell32.lib               # Needed for SHGetKnownFolderPath
)

target_compile_options(GameLauncher PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/WX>          # Treat warnings as errors
    $<$<CXX_COMPILER_ID:MSVC>:/wd4100>     # C4100: unreferenced formal parameter
    $<$<CXX_COMPILER_ID:MSVC>:/wd4127>     # C4127: conditional expression is constant
)

# Suppress specific warnings from third-party headers for specific targets
# Core target (Abseil warnings)
# target_compile_options(core PRIVATE
#     $<$<CXX_COMPILER_ID:MSVC>:/wd4127> # C4127: conditional expression is constant
# )
# GameLauncher target (CEF warnings)

target_include_directories(GameLauncher PUBLIC
    ${PROJECT_SOURCE_DIR}      # Add project root for includes like "cef_integration/..."
    ${cef_binary_SOURCE_DIR}  # Root directory containing cef_app.h, etc.
    ${CURL_INCLUDE_DIRS}     # Add curl include directories
)

target_link_directories(GameLauncher PRIVATE
    "${cef_binary_SOURCE_DIR}/Release"  # Directory containing libcef.lib
    # Add Debug directory if needed for Debug builds, though Release is common
)

# Windows specific settings
if(WIN32)
    target_link_libraries(GameLauncher PRIVATE
        User32.lib  # Often needed for basic windowing/message loop
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
    set(CEF_LOCALES_DIR "${cef_binary_SOURCE_DIR}/Resources/locales")

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
        "${cef_binary_SOURCE_DIR}/Resources/chrome_100_percent.pak"
        "${cef_binary_SOURCE_DIR}/Resources/chrome_200_percent.pak"
        "${cef_binary_SOURCE_DIR}/Resources/icudtl.dat"
        "${cef_binary_SOURCE_DIR}/Resources/resources.pak"
        "${cef_binary_SOURCE_DIR}/Release/v8_context_snapshot.bin" # Moved from Resources to Release
    )

    message(STATUS "CEF Source Directory (cef_binary_SOURCE_DIR): ${cef_binary_SOURCE_DIR}")

    # Use a custom command to copy resources as cef_distribute_resources is problematic
    add_custom_command(
        TARGET GameLauncher POST_BUILD
        # Copy essential files individually using copy_if_different
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/chrome_elf.dll"              "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/d3dcompiler_47.dll"         "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/libcef.dll"                 "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/libEGL.dll"                 "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/libGLESv2.dll"              "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/vk_swiftshader.dll"         "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/vk_swiftshader_icd.json"    "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/vulkan-1.dll"               "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Resources/chrome_100_percent.pak" "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Resources/chrome_200_percent.pak" "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Resources/icudtl.dat"             "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Resources/resources.pak"          "$<TARGET_FILE_DIR:GameLauncher>"
        # snapshot_blob.bin IS MISSING from this CEF distribution (${cef_binary_SOURCE_DIR}/Resources AND /Release). Build will succeed, but may cause runtime V8 errors.
        # COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Resources/snapshot_blob.bin"      "$<TARGET_FILE_DIR:GameLauncher>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${cef_binary_SOURCE_DIR}/Release/v8_context_snapshot.bin"  "$<TARGET_FILE_DIR:GameLauncher>" # Moved from Resources to Release
        # Copy locales directory
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${cef_binary_SOURCE_DIR}/Resources/locales"
            "$<TARGET_FILE_DIR:GameLauncher>/locales" # Ensure destination is quoted
        COMMENT "Copying CEF resources (manual, individual files, snapshot_blob.bin missing)"
        VERBATIM
    )

endif() # End of WIN32 specific settings

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
