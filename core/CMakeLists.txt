# core/CMakeLists.txt

# Define the 'core' library target
# Initially, it will be empty or contain placeholder source files.
# Add source files as they are developed.
add_library(core STATIC # Or SHARED if preferred/needed
    # Add C++ source files for the core module here, e.g.:
    # src/authentication.cpp
    # include/game_launcher/core/authentication.h
)

# Specify include directories for this target
# PUBLIC: Consumers linking 'core' also get these includes
# PRIVATE: Only used for building 'core' itself
# INTERFACE: Only consumers linking 'core' get these includes
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # Include dir within core
    $<INSTALL_INTERFACE:include> # Where headers are installed
)

# Link against dependencies if needed (e.g., Abseil)
# target_link_libraries(core PRIVATE Abseil::status)

message(STATUS "Configuring core library target")

