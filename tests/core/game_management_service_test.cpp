#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "include/base/cef_logging.h" // Use CEF logging
#include "game_launcher/core/basic_game_management_service.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include <filesystem>
#include <vector>
#include <string>

// Helper function to get the path to the test data directory.
// Assumes tests are run from build/tests/ or similar.
std::filesystem::path GetTestDataPath() {
    // This relative path assumes the test executable is somewhere like 
    // build/tests/Debug/CoreTests.exe and the data is in tests/data/
    // Adjust if your build/run setup differs.
    std::filesystem::path p = "../../tests/data/games.json";
    LOG(INFO) << "Looking for test data at: " << std::filesystem::absolute(p).string();
    return p;
}

namespace game_launcher {
namespace core {

class BasicGameManagementServiceTest : public ::testing::Test {
protected:
    // Initialize service with the path to our test JSON file.
    BasicGameManagementServiceTest() : svc_(GetTestDataPath()) {}

    BasicGameManagementService svc_;
};

TEST_F(BasicGameManagementServiceTest, GetInstalledGames_LoadsCorrectly) {
    auto result = svc_.GetInstalledGames();
    ASSERT_TRUE(result.ok());
    std::vector<GameInfo> games = *result;
    ASSERT_EQ(games.size(), 5); // Expect 5 games now (zero, alpha, bad_exe, calc, notepad)

    // Check details of the first game (order might vary depending on map iteration)
    // It's safer to find by ID if order isn't guaranteed.
    // Let's just verify the count and maybe one known entry for simplicity.
    bool found_game_zero = false;
    bool found_game_alpha = false;
    bool found_sys_calc = false;
    for (const auto& game : games) {
        if (game.id == "game_zero") {
            found_game_zero = true;
            EXPECT_EQ(game.name, "Zero Wing");
        } else if (game.id == "game_alpha") {
            found_game_alpha = true;
            EXPECT_EQ(game.name, "Alpha Centauri");
        } else if (game.id == "sys_calc") {
            found_sys_calc = true;
            EXPECT_EQ(game.name, "Calculator");
            EXPECT_EQ(game.install_path, "C:/Windows/System32");
            EXPECT_EQ(game.executable_path, "calc.exe");
        }
    }
    EXPECT_TRUE(found_game_zero);
    EXPECT_TRUE(found_game_alpha);
    EXPECT_TRUE(found_sys_calc);
}

TEST_F(BasicGameManagementServiceTest, GetGameDetails_Found) {
    auto result = svc_.GetGameDetails("game_alpha");
    ASSERT_TRUE(result.ok());
    GameInfo game = *result;
    EXPECT_EQ(game.id, "game_alpha");
    EXPECT_EQ(game.name, "Alpha Centauri");
    EXPECT_EQ(game.version, "4.0");
}

TEST_F(BasicGameManagementServiceTest, GetGameDetails_NotFound) {
    auto result = svc_.GetGameDetails("any_id");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), absl::StatusCode::kNotFound);
}

TEST_F(BasicGameManagementServiceTest, LaunchGame_Success) {
    // Note: This test only verifies that the service *attempts* to launch
    // for a valid game ID and existing executable path structure in the JSON.
    // It doesn't guarantee the process *actually* started successfully, 
    // as the paths might not point to a real executable on the test machine.
    // We check for OkStatus, indicating CreateProcess was called without 
    // immediate path or setup errors detectable by our code.
    auto status = svc_.LaunchGame("sys_calc"); // Use Calculator for a known executable
    // In a real scenario where the path MUST exist, we might create dummy files/dirs
    // or mock filesystem interactions.
    EXPECT_TRUE(status.ok()) << "LaunchGame failed with status: " << status;
}

TEST_F(BasicGameManagementServiceTest, LaunchGame_GameNotFound) {
    auto status = svc_.LaunchGame("non_existent_game_id");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), absl::StatusCode::kNotFound);
}

TEST_F(BasicGameManagementServiceTest, LaunchGame_ExecutableNotFound) {
    // This uses the 'game_bad_exe' entry we added to games.json
    auto status = svc_.LaunchGame("game_bad_exe");
    EXPECT_FALSE(status.ok());
    // Our LaunchGame implementation checks std::filesystem::exists
    EXPECT_EQ(status.code(), absl::StatusCode::kNotFound);
}

}  // namespace core
}  // namespace game_launcher
