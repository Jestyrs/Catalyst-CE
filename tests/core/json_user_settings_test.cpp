#include "game_launcher/core/json_user_settings.h"
#include <gtest/gtest.h>
#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <filesystem>
#include <fstream>
#include <memory>

namespace game_launcher::core {

// Helper function to get a temporary file path
std::filesystem::path GetTemporaryFilePath() {
    return std::filesystem::temp_directory_path() / "test_settings.json";
}

class JsonUserSettingsTest : public ::testing::Test {
protected:
    std::filesystem::path test_file_path_;
    std::unique_ptr<JsonUserSettings> settings_;

    void SetUp() override {
        test_file_path_ = GetTemporaryFilePath();
        // Ensure the file doesn't exist before each test
        std::filesystem::remove(test_file_path_);
        settings_ = std::make_unique<JsonUserSettings>(test_file_path_);
    }

    void TearDown() override {
        // Clean up the file after each test
        std::filesystem::remove(test_file_path_);
    }

    // Helper to create a malformed JSON file
    void CreateMalformedFile() {
        std::ofstream ofs(test_file_path_);
        ofs << "{ \"key\": \"value\" , "; // Malformed JSON
        ofs.close();
    }
};

TEST_F(JsonUserSettingsTest, Initialization) {
    // Test that the object is created and the file doesn't exist yet
    EXPECT_FALSE(std::filesystem::exists(test_file_path_));
    EXPECT_FALSE(settings_->IsDirty());
}

TEST_F(JsonUserSettingsTest, LoadNonExistentFile) {
    // Test loading when the file doesn't exist - should succeed and be empty
    EXPECT_TRUE(settings_->LoadSettings().ok());
    EXPECT_FALSE(settings_->HasKey("any_key"));
    EXPECT_FALSE(settings_->IsDirty());
}

TEST_F(JsonUserSettingsTest, SaveCreatesFile) {
    EXPECT_TRUE(settings_->SetString("test_key", "test_value").ok());
    EXPECT_TRUE(settings_->IsDirty());
    EXPECT_TRUE(settings_->SaveSettings().ok());
    EXPECT_TRUE(std::filesystem::exists(test_file_path_));
    EXPECT_FALSE(settings_->IsDirty());
}

TEST_F(JsonUserSettingsTest, LoadMalformedFile) {
    CreateMalformedFile();
    absl::Status status = settings_->LoadSettings();
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), absl::StatusCode::kDataLoss); 
    // Settings should be empty/default after failed load
    EXPECT_FALSE(settings_->HasKey("key")); 
}

// Add more tests here for Get/Set/Remove/Clear/Save/Load logic...

} // namespace game_launcher::core
