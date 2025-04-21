# CMakeLists.txt (Root)

cmake_minimum_required(VERSION 3.21) # Specify minimum CMake version

project(GameLauncher VERSION 0.1.0 LANGUAGES CXX)

# Set the C++ standard required for the project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer explicit standard features

# --- Project Structure ---
# Add subdirectories containing their own CMakeLists.txt files
add_subdirectory(core)
add_subdirectory(cef_integration)
add_subdirectory(platform)
add_subdirectory(tests)
# ui/ - Typically contains static assets, might not need its own CMakeLists unless processing is required.
# build/ - Build output directory, managed by CMake.
# docs/ - Documentation, not typically part of the CMake build process.

# --- Executable Definition (Placeholder) ---
# Example: Define the main launcher executable later
# add_executable(GameLauncherLauncher main.cpp) # Needs a source file
# target_link_libraries(GameLauncherLauncher PRIVATE core cef_integration platform) # Link against modules

# --- CEF Configuration (Placeholder) ---
# TODO: Implement CEF discovery (find_package or FetchContent)
# TODO: Link libcef, libcef_dll_wrapper, and ensure cef_sandbox is linked statically
# find_package(CEF REQUIRED)
# target_link_libraries(GameLauncherLauncher PRIVATE CEF::libcef_dll_wrapper)
# if(WIN32)
#   target_link_libraries(GameLauncherLauncher PRIVATE CEF::cef_sandbox) # Static linking for sandbox
# endif()

# --- Dependency Management (Placeholder) ---
# TODO: Configure dependency management (e.g., FetchContent, Conan)
# Example using FetchContent for GoogleTest:
# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   GIT_REPOSITORY https://github.com/google/googletest.git
#   GIT_TAG    release-1.12.1 # Or a specific commit/tag
# )
# FetchContent_MakeAvailable(googletest)

# --- Build Types ---
# Standard build types (Debug, Release) are usually handled by CMake/Generators.
# We can add custom flags if needed:
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address") # Example: Add ASan for Debug builds
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3") # Example: Ensure high optimization for Release

# --- Testing Configuration (Placeholder) ---
# TODO: Enable testing and add tests discovered from the 'tests' subdirectory
# enable_testing()
# add_test(NAME CoreUnitTests COMMAND ...)

# --- Installation Rules (Placeholder) ---
# TODO: Define installation rules for the executable, libraries, CEF resources, and UI assets
# install(TARGETS GameLauncherLauncher DESTINATION bin)
# install(FILES ${CEF_RESOURCES} DESTINATION bin)
# install(DIRECTORY ui/ DESTINATION share/GameLauncher/ui)

message(STATUS "Project: ${PROJECT_NAME} configured.")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")

