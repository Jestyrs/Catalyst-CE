// tests/core/user_settings_test.cpp

#include "game_launcher/core/user_settings.h"
#include "game_launcher/core/json_user_settings.h"    // For JsonUserSettings
#include <gtest/gtest.h>
#include <filesystem>
#include <memory> // For std::unique_ptr

using namespace game_launcher::core;

// Test Fixture for Settings implementations
class SettingsTest : public ::testing::TestWithParam<std::unique_ptr<IUserSettings>(*)()> {
protected:
    std::unique_ptr<IUserSettings> settings_;

    void SetUp() override {
        auto factory = GetParam();
        settings_ = factory(); // Create instance using factory
        ASSERT_TRUE(settings_->LoadSettings());
    }
};

// Test suite for basic Get/Set operations applicable to any implementation
TEST_P(SettingsTest, StringLifecycle) {
    const std::string key = "string_key";
    const std::string value1 = "value1";
    const std::string value2 = "value2";

    // Initial state
    EXPECT_FALSE(settings_->GetString(key).has_value());

    // Set value
    settings_->SetString(key, value1);
    ASSERT_TRUE(settings_->GetString(key).has_value());
    EXPECT_EQ(settings_->GetString(key).value(), value1);

    // Overwrite value
    settings_->SetString(key, value2);
    ASSERT_TRUE(settings_->GetString(key).has_value());
    EXPECT_EQ(settings_->GetString(key).value(), value2);

    // Check type mismatch (optional, depends on desired strictness)
    // settings_->SetInt(key, 100); // Set as different type
    // EXPECT_FALSE(settings_->GetString(key).has_value());
}

TEST_P(SettingsTest, IntLifecycle) {
    const std::string key = "int_key";
    const int value1 = 42;
    const int value2 = -100;

    EXPECT_FALSE(settings_->GetInt(key).has_value());
    settings_->SetInt(key, value1);
    ASSERT_TRUE(settings_->GetInt(key).has_value());
    EXPECT_EQ(settings_->GetInt(key).value(), value1);

    settings_->SetInt(key, value2);
    ASSERT_TRUE(settings_->GetInt(key).has_value());
    EXPECT_EQ(settings_->GetInt(key).value(), value2);
}

TEST_P(SettingsTest, BoolLifecycle) {
    const std::string key = "bool_key";

    EXPECT_FALSE(settings_->GetBool(key).has_value());
    settings_->SetBool(key, true);
    ASSERT_TRUE(settings_->GetBool(key).has_value());
    EXPECT_TRUE(settings_->GetBool(key).value());

    settings_->SetBool(key, false);
    ASSERT_TRUE(settings_->GetBool(key).has_value());
    EXPECT_FALSE(settings_->GetBool(key).value());
}

// --- Instantiating the tests for InMemoryUserSettings ---

// Factory function specifically for the test fixture
std::unique_ptr<IUserSettings> InMemoryFactory() {
    return CreateInMemoryUserSettings();
}

INSTANTIATE_TEST_SUITE_P(
    InMemory,
    SettingsTest,
    ::testing::Values(InMemoryFactory)
);

// --- Specific Tests for JsonUserSettings ---
class JsonSettingsTest : public ::testing::Test {
protected:
    const std::filesystem::path temp_file_ = 
        std::filesystem::temp_directory_path() / "gl_test_settings.json";

    void SetUp() override {
        // Ensure clean state before each test
        if (std::filesystem::exists(temp_file_)) {
            std::filesystem::remove(temp_file_);
        }
    }

    void TearDown() override {
        // Clean up after each test
         if (std::filesystem::exists(temp_file_)) {
            std::filesystem::remove(temp_file_);
        }
    }
};

TEST_F(JsonSettingsTest, FileCreationAndPersistence) {
    {
        JsonUserSettings settings(temp_file_);
        // Load should create the file if it doesn't exist
        EXPECT_TRUE(settings.LoadSettings());
        EXPECT_TRUE(std::filesystem::exists(temp_file_));

        settings.SetString("persistent_key", "persistent_value");
        settings.SetInt("persistent_int", 999);
        EXPECT_TRUE(settings.SaveSettings()); // Explicit save
    }

    // Create a new instance to load the saved data
    JsonUserSettings loaded_settings(temp_file_);
    EXPECT_TRUE(loaded_settings.LoadSettings());
    ASSERT_TRUE(loaded_settings.GetString("persistent_key").has_value());
    EXPECT_EQ(loaded_settings.GetString("persistent_key").value(), "persistent_value");
    ASSERT_TRUE(loaded_settings.GetInt("persistent_int").has_value());
    EXPECT_EQ(loaded_settings.GetInt("persistent_int").value(), 999);
}

TEST_F(JsonSettingsTest, LoadInvalidJsonResets) {
    // Create an invalid JSON file
    {
        std::ofstream ofs(temp_file_);
        ofs << "this is not json{";
    }

    JsonUserSettings settings(temp_file_);
    // Load should fail but reset to an empty state
    EXPECT_FALSE(settings.LoadSettings()); 
    settings.SetString("after_reset", "should_work");
    EXPECT_TRUE(settings.GetString("after_reset").has_value());
    // Saving should now write a valid empty or corrected file
    EXPECT_TRUE(settings.SaveSettings());

    // Verify the saved file is now valid (contains "after_reset")
     JsonUserSettings reloaded_settings(temp_file_);
     EXPECT_TRUE(reloaded_settings.LoadSettings());
     EXPECT_EQ(reloaded_settings.GetString("after_reset").value(), "should_work");
}

/* Commented out for now as JsonUserSettings needs <filesystem> support
// --- Instantiating the tests for JsonUserSettings ---
std::unique_ptr<IUserSettings> JsonFactory() {
    // NOTE: This shares the same temp file across all tests in the suite instance.
    // Might need a more robust fixture if tests interfere.
    static const std::filesystem::path temp_file = 
        std::filesystem::temp_directory_path() / "gl_test_param_settings.json";
     if (std::filesystem::exists(temp_file)) {
            std::filesystem::remove(temp_file);
     }
    return std::make_unique<JsonUserSettings>(temp_file);
}

INSTANTIATE_TEST_SUITE_P(
    Json,
    SettingsTest,
    ::testing::Values(JsonFactory)
);
*/

// Google Test entry point (optional if linking GTest::gtest_main)
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }

