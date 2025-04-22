# core/CMakeLists.txt

# Require C++20
set(CMAKE_CXX_STANDARD 20) # Changed from 17 to 20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Ensure standard conformance, disable GNU extensions

# Define the 'core' library target
# Initially, it will be empty or contain placeholder source files.
# Add source files as they are developed.
set(CORE_SOURCES
    src/user_settings.cpp
    src/json_user_settings.cpp
    src/background_task_manager.cpp
    src/authentication_manager.cpp # Add the new source file
    src/basic_game_management_service.cpp 
    src/basic_ipc_service.cpp
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
    ${cef_binary_SOURCE_DIR} # Add CEF include directory
)
# Add third_party include for nlohmann/json
# Must be PUBLIC because json_user_settings.h includes json.hpp
target_include_directories(core PUBLIC 
    ${CMAKE_SOURCE_DIR}/third_party # Use absolute path for robustness
)

# Link against Abseil for status handling
target_link_libraries(core PUBLIC
    absl::base        # Fundamental Abseil utilities
    absl::status      # For absl::Status
    absl::statusor    # For absl::StatusOr
    absl::strings     # For StrCat etc.
    absl::flat_hash_map # For hash map/set implementations
    # --- REMOVED: absl::common --- Target does not exist in this Abseil version
)

# Link dependencies
target_link_libraries(core PUBLIC
    # Public dependencies (needed by headers)
    absl::status
    absl::statusor
    nlohmann_json::nlohmann_json
    PRIVATE
    # Private dependencies (only needed by implementation)
    absl::strings
    # absl::log # Removed - Using CEF logging now
)

message(STATUS "Configuring core library target")
