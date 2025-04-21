# cef_integration/CMakeLists.txt

# Define the 'cef_integration' library target
add_library(cef_integration STATIC # Or SHARED
    # Add C++ source files for the CEF integration layer here, e.g.:
    # src/cef_app.cpp
    # src/cef_client.cpp
    # include/game_launcher/cef/cef_app.h
)

# Specify include directories
target_include_directories(cef_integration PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link against the 'core' library
# Also link against CEF libraries once configured in the root CMakeLists.txt
target_link_libraries(cef_integration PUBLIC
    core # Depends on the core logic
    # PUBLIC/PRIVATE CEF::libcef_dll_wrapper # Example linking CEF
)

# Required include for CEF (placeholder - actual target comes from root CMake)
# target_include_directories(cef_integration PRIVATE ${CEF_INCLUDE_DIRS})

message(STATUS "Configuring cef_integration library target")

