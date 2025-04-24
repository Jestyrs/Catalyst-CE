# platform/CMakeLists.txt

# Define the 'platform' library target
add_library(platform STATIC # Or SHARED
    # Add platform-specific C++ source files here, e.g.:
    src/placeholder.cpp # Placeholder to allow build
    # src/windows/win_utils.cpp
    # src/macos/mac_utils.mm
    # src/linux/linux_utils.cpp
    # include/game_launcher/platform/platform.h
)

# Specify include directories
target_include_directories(platform PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Conditional compilation/linking based on OS
if(WIN32)
    # Add Windows-specific sources or link libraries
    # target_sources(platform PRIVATE src/windows/win_utils.cpp)
    # target_link_libraries(platform PRIVATE Shlwapi.lib) # Example Windows lib
elseif(APPLE)
    # Add macOS-specific sources or link frameworks
    # target_sources(platform PRIVATE src/macos/mac_utils.mm)
    # target_link_libraries(platform PRIVATE "-framework Cocoa") # Example macOS framework
elseif(UNIX AND NOT APPLE)
    # Add Linux-specific sources or link libraries
    # target_sources(platform PRIVATE src/linux/linux_utils.cpp)
    # target_link_libraries(platform PRIVATE some_linux_lib) # Example Linux lib
endif()

message(STATUS "Configuring platform library target")
