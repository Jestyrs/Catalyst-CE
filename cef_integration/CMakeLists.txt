# cef_integration/CMakeLists.txt

# Define the 'cef_integration' library target
add_library(cef_integration STATIC # Or SHARED
    # Add C++ source files for the CEF integration layer here, e.g.:
    src/placeholder.cpp # Placeholder, keep for now
    src/launcher_message_router_handler.cpp
    src/launcher_client.cpp
    src/launcher_app.cpp
    src/launcher_render_process_handler.cpp
    src/game_launcher_api_handler.cpp # Added V8 API handler
    src/cef_json_utils.cpp # Added source
)

# Specify include directories
target_include_directories(cef_integration PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${cef_binary_SOURCE_DIR}/include # Needed for our code to find cef_*.h
    ${cef_binary_SOURCE_DIR}         # Needed for internal CEF includes like include/base/...
)

# Define NOMINMAX to prevent Windows header conflicts (min/max macros)
target_compile_definitions(cef_integration PRIVATE NOMINMAX)

# Add necessary preprocessor definitions for CEF
target_compile_definitions(cef_integration PUBLIC
    USING_CEF_SHARED  # Required when linking against the shared libcef library (DLL)
)

# Link against the 'core' library
# Also link against CEF libraries once configured in the root CMakeLists.txt
target_link_libraries(cef_integration PUBLIC
    libcef_dll_wrapper # Link PUBLIC because our headers include CEF headers
    core               # Link core PUBLIC because its interface (IIPCService) is used by cef_integration
    # Link Abseil PUBLIC because core's public interface uses these types
    absl::status                 
    absl::statusor               
    absl::strings                
    # Consider making others PUBLIC only if cef_integration headers expose their types
    # Assuming headers primarily use standard types, pointers, or forward declarations
    # Keep nlohmann_json and absl PRIVATE unless proven otherwise
    PRIVATE
    nlohmann_json::nlohmann_json # For parsing JSON requests
    # absl::log                    # Removed: Use CEF logging within this layer
)

message(STATUS "Configuring cef_integration library target")

# Add /WX only to this target
target_compile_options(cef_integration PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/WX>)

# Suppress specific warnings from included headers (CEF, Abseil)
target_compile_options(cef_integration PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/wd4100> # C4100: unreferenced formal parameter (often in CEF/Abseil)
    $<$<CXX_COMPILER_ID:MSVC>:/wd4127> # C4127: conditional expression is constant (often in Abseil)
)
