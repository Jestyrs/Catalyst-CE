# core/CMakeLists.txt

include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3 # Use a specific stable version
)
FetchContent_MakeAvailable(nlohmann_json)

# Require C++20
set(CMAKE_CXX_STANDARD 20) # Changed from 17 to 20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensure standard conformance, disable GNU extensions

# Define the 'core' library target
# Initially, it will be empty or contain placeholder source files.
# Add source files as they are developed.
set(CORE_SOURCES
    src/user_settings.cpp
    src/core_ipc_service.cpp            # Added from game_launcher_core
    src/json_user_settings.cpp
    src/background_task_manager.cpp
    src/authentication_manager.cpp # Add the new source file
    src/basic_game_management_service.cpp 
    src/basic_ipc_service.cpp
    src/manifest_parser.cpp             # Added for manifest parsing
    src/network_utils.cpp             # Added for downloading files
    src/user_profile.cpp # Add the new source file
    src/app_settings.cpp # Added now
    # Add other core source files here
)

add_library(core STATIC ${CORE_SOURCES})

# Specify include directories for this target
# PUBLIC: Consumers linking 'core' also get these includes
# PRIVATE: Only used for building 'core' itself
# INTERFACE: Only consumers linking 'core' get these includes
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # Include dir within core
    $<INSTALL_INTERFACE:include> # Where headers are installed
    PRIVATE
    ${cef_binary_SOURCE_DIR} # Add CEF root include directory
    ${picosha2_SOURCE_DIR}   # Added: Include dir for picosha2.h
)

# Link dependencies
target_link_libraries(core 
    PUBLIC # Interface dependencies (needed by headers/consumers)
    # Public dependencies (needed by headers)
    absl::status
    absl::statusor
    absl::flat_hash_map         # Moved from PRIVATE
    PRIVATE
    # Implementation dependencies (only needed by core source files)
    absl::base                  # Fundamental Abseil utilities
    absl::cleanup               # Added - For absl::MakeCleanup
    absl::log                   # Added - For LOG macros
    absl::strings               # For StrCat etc.
    absl::synchronization       # Used by CoreIPCService (threads/mutexes)
    nlohmann_json::nlohmann_json # Moved from PUBLIC
    CURL::libcurl                # Moved from PUBLIC
    libcef_dll_wrapper # Link against CEF wrapper for logging utils
)

message(STATUS "Configuring core library target")

# Add /WX only to this target
target_compile_options(core PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/WX>)

# Suppress specific warnings from included third-party headers (like Abseil)
# that would otherwise cause errors due to /WX being enabled for 'core'.
target_compile_options(core PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/wd4127 /wd4100>)

# Disable specific MSVC warnings if necessary
if(MSVC)
    target_compile_options(core PRIVATE /W4 /WX)
    # Disable padding warning often triggered by external libs like Abseil
    target_compile_options(core PRIVATE /wd4324)
endif()
