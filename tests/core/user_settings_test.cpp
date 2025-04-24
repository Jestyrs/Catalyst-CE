// tests/core/user_settings_test.cpp

#include "game_launcher/core/user_settings.h"
#include "game_launcher/core/json_user_settings.h"    // For JsonUserSettings
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream> // Needed for std::ofstream
#include <memory> // For std::unique_ptr
#include <absl/status/status.h> // For status checks
#include <absl/status/statusor.h> // For status checks

using namespace game_launcher::core;

// Test Fixture for Settings implementations
class SettingsTest : public ::testing::TestWithParam<std::unique_ptr<IUserSettings>(*)()> {
protected:
    std::unique_ptr<IUserSettings> settings_;

    void SetUp() override {
        auto factory = GetParam();
        settings_ = factory(); // Create instance using factory
        ASSERT_TRUE(settings_->LoadSettings().ok());
    }
};

// Test suite for basic Get/Set operations applicable to any implementation
TEST_P(SettingsTest, StringLifecycle) {
    const std::string key = "string_key";
    const std::string value1 = "value1";
    const std::string value2 = "value2";

    // Initial state
    absl::StatusOr<std::string> initial_value = settings_->GetString(key);
    EXPECT_FALSE(initial_value.ok());

    // Set value
    EXPECT_TRUE(settings_->SetString(key, value1).ok());
    absl::StatusOr<std::string> value_status = settings_->GetString(key);
    ASSERT_TRUE(value_status.ok()) << value_status.status();
    EXPECT_EQ(value_status.value(), value1);

    // Overwrite value
    EXPECT_TRUE(settings_->SetString(key, value2).ok());
    value_status = settings_->GetString(key);
    ASSERT_TRUE(value_status.ok()) << value_status.status();
    EXPECT_EQ(value_status.value(), value2);

    // Check type mismatch (optional, depends on desired strictness)
    // settings_->SetInt(key, 100); // Set as different type
    // EXPECT_FALSE(settings_->GetString(key).has_value());
}

TEST_P(SettingsTest, IntLifecycle) {
    const std::string key = "int_key";
    const int value1 = 42;
    const int value2 = -100;

    absl::StatusOr<int> initial_value = settings_->GetInt(key);
    EXPECT_FALSE(initial_value.ok());
    EXPECT_TRUE(settings_->SetInt(key, value1).ok());
    absl::StatusOr<int> value_status = settings_->GetInt(key);
    ASSERT_TRUE(value_status.ok()) << value_status.status();
    EXPECT_EQ(value_status.value(), value1);

    EXPECT_TRUE(settings_->SetInt(key, value2).ok());
    value_status = settings_->GetInt(key);
    ASSERT_TRUE(value_status.ok()) << value_status.status();
    EXPECT_EQ(value_status.value(), value2);
}

TEST_P(SettingsTest, BoolLifecycle) {
    const std::string key = "bool_key";

    absl::StatusOr<bool> initial_value = settings_->GetBool(key);
    EXPECT_FALSE(initial_value.ok());
    EXPECT_TRUE(settings_->SetBool(key, true).ok());
    absl::StatusOr<bool> value_status = settings_->GetBool(key);
    ASSERT_TRUE(value_status.ok()) << value_status.status();
    EXPECT_TRUE(value_status.value());

    EXPECT_TRUE(settings_->SetBool(key, false).ok());
    value_status = settings_->GetBool(key);
    ASSERT_TRUE(value_status.ok()) << value_status.status();
    EXPECT_FALSE(value_status.value());
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
        EXPECT_TRUE(settings.LoadSettings().ok());
        EXPECT_TRUE(std::filesystem::exists(temp_file_));

        EXPECT_TRUE(settings.SetString("persistent_key", "persistent_value").ok());
        EXPECT_TRUE(settings.SetInt("persistent_int", 999).ok());
        EXPECT_TRUE(settings.SaveSettings().ok()); // Explicit save
    }

    // Create a new instance to load the saved data
    JsonUserSettings loaded_settings(temp_file_);
    EXPECT_TRUE(loaded_settings.LoadSettings().ok());
    absl::StatusOr<std::string> loaded_value = loaded_settings.GetString("persistent_key");
    ASSERT_TRUE(loaded_value.ok()) << loaded_value.status();
    EXPECT_EQ(loaded_value.value(), "persistent_value");
    absl::StatusOr<int> loaded_int = loaded_settings.GetInt("persistent_int");
    ASSERT_TRUE(loaded_int.ok()) << loaded_int.status();
    EXPECT_EQ(loaded_int.value(), 999);
}

TEST_F(JsonSettingsTest, LoadInvalidJsonResets) {
    // Create an invalid JSON file
    {
        std::ofstream ofs(temp_file_);
        ofs << "this is not json{";
    }

    JsonUserSettings settings(temp_file_);
    // Load should fail but reset to an empty state
    EXPECT_FALSE(settings.LoadSettings().ok()); 
    EXPECT_TRUE(settings.SetString("after_reset", "should_work").ok());
    absl::StatusOr<std::string> after_reset_value = settings.GetString("after_reset");
    EXPECT_TRUE(after_reset_value.ok()) << after_reset_value.status();
    // Saving should now write a valid empty or corrected file
    EXPECT_TRUE(settings.SaveSettings().ok());

    // Verify the saved file is now valid (contains "after_reset")
     JsonUserSettings reloaded_settings(temp_file_);
     EXPECT_TRUE(reloaded_settings.LoadSettings().ok());
     absl::StatusOr<std::string> reloaded_value = reloaded_settings.GetString("after_reset");
     EXPECT_TRUE(reloaded_value.ok()) << reloaded_value.status();
     EXPECT_EQ(reloaded_value.value(), "should_work");
}

TEST_F(JsonSettingsTest, HasKeyCheck) {
    {
        JsonUserSettings settings(temp_file_);
        EXPECT_TRUE(settings.SetString("key_exists", "value_exists").ok());
        EXPECT_TRUE(settings.SaveSettings().ok());
    }

    JsonUserSettings loaded_settings(temp_file_);
    EXPECT_TRUE(loaded_settings.LoadSettings().ok());
    EXPECT_TRUE(loaded_settings.HasKey("key_exists"));
    EXPECT_FALSE(loaded_settings.HasKey("key_does_not_exist"));
}

TEST_F(JsonSettingsTest, RemoveKeyPersistence) {
    {
        JsonUserSettings settings(temp_file_);
        EXPECT_TRUE(settings.SetString("key_to_keep", "value1").ok());
        EXPECT_TRUE(settings.SetInt("key_to_remove", 123).ok());
        EXPECT_TRUE(settings.SaveSettings().ok());
    }

    {
        JsonUserSettings settings_reloaded(temp_file_);
        EXPECT_TRUE(settings_reloaded.LoadSettings().ok());
        EXPECT_TRUE(settings_reloaded.HasKey("key_to_remove")); // Verify it exists before removal
        EXPECT_TRUE(settings_reloaded.RemoveKey("key_to_remove").ok());
        EXPECT_TRUE(settings_reloaded.RemoveKey("key_does_not_exist").ok()); // Removing non-existent should be OK
        EXPECT_TRUE(settings_reloaded.SaveSettings().ok()); // Save after removal
    }

    JsonUserSettings final_settings(temp_file_);
    EXPECT_TRUE(final_settings.LoadSettings().ok());
    EXPECT_TRUE(final_settings.HasKey("key_to_keep"));
    EXPECT_FALSE(final_settings.HasKey("key_to_remove"));
    absl::StatusOr<int> removed_value = final_settings.GetInt("key_to_remove");
    EXPECT_FALSE(removed_value.ok());
}

TEST_F(JsonSettingsTest, ClearPersistence) {
    {
        JsonUserSettings settings(temp_file_);
        EXPECT_TRUE(settings.SetString("key1", "v1").ok());
        EXPECT_TRUE(settings.SetInt("key2", 2).ok());
        EXPECT_TRUE(settings.SetBool("key3", true).ok());
        EXPECT_TRUE(settings.SaveSettings().ok());
    }

    {
        JsonUserSettings settings_reloaded(temp_file_);
        EXPECT_TRUE(settings_reloaded.LoadSettings().ok());
        EXPECT_TRUE(settings_reloaded.HasKey("key1")); // Verify exists
        EXPECT_TRUE(settings_reloaded.Clear().ok());
        EXPECT_FALSE(settings_reloaded.HasKey("key1")); // Should be gone from memory immediately
        EXPECT_TRUE(settings_reloaded.SaveSettings().ok()); // Save the cleared state
    }

    JsonUserSettings final_settings(temp_file_);
    EXPECT_TRUE(final_settings.LoadSettings().ok());
    EXPECT_FALSE(final_settings.HasKey("key1"));
    EXPECT_FALSE(final_settings.HasKey("key2"));
    EXPECT_FALSE(final_settings.HasKey("key3"));
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
