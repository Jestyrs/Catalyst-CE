# core/CMakeLists.txt

# Require C++17 for std::filesystem etc.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the 'core' library target
# Initially, it will be empty or contain placeholder source files.
# Add source files as they are developed.
add_library(core STATIC # Or SHARED if preferred/needed
    # Add C++ source files for the core module here, e.g.:
    src/authentication.cpp
    src/background_task_manager.cpp
    src/game_management.cpp
    src/ipc_service_layer.cpp
    src/user_settings.cpp
    src/json_user_settings.cpp # Added new user settings implementation
    # include/game_launcher/core/authentication.h # Headers typically not listed here
)

# Specify include directories for this target
# PUBLIC: Consumers linking 'core' also get these includes
# PRIVATE: Only used for building 'core' itself
# INTERFACE: Only consumers linking 'core' get these includes
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # Include dir within core
    $<INSTALL_INTERFACE:include> # Where headers are installed
)
# Add third_party include for nlohmann/json
target_include_directories(core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# Link against dependencies if needed (e.g., Abseil)
# target_link_libraries(core PRIVATE Abseil::status)

message(STATUS "Configuring core library target")
