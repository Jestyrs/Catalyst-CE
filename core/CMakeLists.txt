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
    src/game_management.cpp             # Added from game_launcher_core
    src/json_user_settings.cpp
    src/background_task_manager.cpp
    src/authentication_manager.cpp # Add the new source file
    src/basic_game_management_service.cpp 
    src/basic_ipc_service.cpp
    src/manifest_parser.cpp             # Added for manifest parsing
    src/network_utils.cpp             # Added for downloading files
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
    include/game_launcher/core
    PRIVATE
    ${cef_binary_SOURCE_DIR}          # For internal CEF includes like include/base/cef_build.h
    ${cef_binary_SOURCE_DIR}/include  # For direct includes like base/cef_logging.h
    # No private includes needed for now
)

# Disable specific warnings treated as errors (/WX)
# C4127: conditional expression is constant (common in templates/macros)
# C4100: unreferenced formal parameter
# C4101: unreferenced local variable
target_compile_options(core PRIVATE
    # MSVC specific flags to disable warnings
    $<$<CXX_COMPILER_ID:MSVC>:/wd4127 /wd4100 /wd4101>
)

# Add necessary compile definitions
target_compile_definitions(core PRIVATE
    USING_CEF_SHARED # Commonly required when using CEF shared library/wrapper
)

# Link dependencies
target_link_libraries(core 
    PUBLIC # Interface dependencies (needed by headers/consumers)
    # Public dependencies (needed by headers)
    absl::status
    absl::statusor
    nlohmann_json::nlohmann_json
    CURL::libcurl             # Link against libcurl for network_utils - Now public
    PRIVATE
    libcef_dll_wrapper          # Link against the CEF wrapper library
    # Implementation dependencies (only needed by core source files)
    absl::base                  # Fundamental Abseil utilities
    absl::strings               # For StrCat etc.
    absl::synchronization       # Used by CoreIPCService (threads/mutexes)
    absl::flat_hash_map         # For hash map/set implementations
)

message(STATUS "Configuring core library target")
