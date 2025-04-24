# tests/CMakeLists.txt

# Note: enable_testing() is usually called in the root CMakeLists.txt

# Find the testing framework (e.g., GoogleTest)
# This assumes GoogleTest has been made available via FetchContent or find_package
# in the root CMakeLists.txt
# find_package(GTest REQUIRED)

# Define test executables
# Example: Unit tests for the 'core' module
set(TEST_SOURCES
    core/user_settings_test.cpp
    # Add other core test source files here
    core/background_task_manager_test.cpp
    core/json_user_settings_test.cpp
    core/authentication_manager_test.cpp
    core/game_management_service_test.cpp
    core/basic_ipc_service_test.cpp
    # core/authentication_test.cpp
    # core/game_management_test.cpp
)

add_executable(CoreTests ${TEST_SOURCES})

# Prevent common Windows header macro conflicts
target_compile_definitions(CoreTests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

# Link tests against the library being tested and the test framework
target_link_libraries(CoreTests PRIVATE
    core              # Library under test
    GTest::gtest_main # GoogleTest main library (includes gtest)
    GTest::gmock      # GoogleMock library (includes gmock)
    absl::base        # Fundamental Abseil utilities
    absl::status      # For absl::Status
    absl::log         # For LOG() macros
    absl::synchronization # For BasicIPCService Mutex
    absl::flat_hash_map   # For BasicIPCService flat_hash_map
    absl::strings         # For BasicIPCService StrCat
)

# Add tests to CTest
include(GoogleTest) # Use CMake's built-in GoogleTest integration
gtest_discover_tests(CoreTests) # Automatically discover tests in the executable

# Optional: Add test using add_test if not using gtest_discover_tests
# add_test(NAME CoreUnitTests COMMAND CoreTests)

message(STATUS "Configuring tests directory: CoreTests executable created.")
