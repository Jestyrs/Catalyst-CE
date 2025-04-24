# main/CMakeLists.txt

# Add the source file for the main executable
# The target 'GameLauncher' is defined in the parent CMakeLists.txt
target_sources(GameLauncher PRIVATE
    main.cpp
)

# Specify include directories needed by main.cpp
# We need access to core and cef_integration includes
# Includes for CEF and Abseil should be inherited via target_link_libraries
target_include_directories(GameLauncher PRIVATE
    # Access to headers from the 'core' module
    ${PROJECT_SOURCE_DIR}/core/include
    # Access to headers from the 'cef_integration' module
    ${PROJECT_SOURCE_DIR}/cef_integration/include
    # REMOVED: Redundant includes, should be inherited from linked targets
    # ${CEF_INCLUDE_DIRS}
    # ${absl_INCLUDE_DIRS}
    # Explicitly add CEF include directory *again* directly to GameLauncher
    # as propagation seems problematic.
    ${cef_binary_SOURCE_DIR}
)
