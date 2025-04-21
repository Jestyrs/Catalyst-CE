# tests/CMakeLists.txt

# Note: enable_testing() is usually called in the root CMakeLists.txt

# Find the testing framework (e.g., GoogleTest)
# This assumes GoogleTest has been made available via FetchContent or find_package
# in the root CMakeLists.txt
# find_package(GTest REQUIRED)

# Define test executables
# Example: Unit tests for the 'core' module
add_executable(CoreTests
    # Add test source files here, e.g.:
    # core/authentication_test.cpp
    # core/game_management_test.cpp
)

# Link tests against the library being tested and the test framework
target_link_libraries(CoreTests PRIVATE
    core      # Library under test
    # GTest::gtest_main # GoogleTest main library
    # GTest::gmock      # GoogleMock if needed
)

# Add tests to CTest
# add_test(NAME CoreUnitTests COMMAND CoreTests)

message(STATUS "Configuring tests directory")

