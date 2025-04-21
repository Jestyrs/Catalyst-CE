// core/src/json_user_settings.cpp
#include "game_launcher/core/json_user_settings.h"
#include <fstream>    // For std::ifstream, std::ofstream
#include <iostream>   // For std::cerr
#include <filesystem> // For create_directories
#include <utility>    // For std::move

namespace game_launcher {
namespace core {

namespace fs = std::filesystem; // Alias for convenience

JsonUserSettings::JsonUserSettings(fs::path settings_file_path)
    : settings_file_path_(std::move(settings_file_path)) {
    // Attempt to load settings immediately upon construction.
    // If loading fails (e.g., file corrupted but exists), log error but continue.
    // If file doesn't exist, LoadSettings will create a default empty JSON object.
    if (!LoadSettings()) {
        std::cerr << "JsonUserSettings: Initial load failed for "
                  << settings_file_path_.string() << ". Using default empty settings." << std::endl;
        // Ensure settings_json_ is a valid (empty) object if loading failed badly
        settings_json_ = nlohmann::json::object();
        dirty_flag_ = true; // Mark as dirty so an empty file is created on first save attempt
    }
}

JsonUserSettings::~JsonUserSettings() {
    // Attempt to save settings on destruction if they are dirty.
    if (dirty_flag_) {
        if (!SaveSettings()) {
             // Log error, but can't do much else in destructor
             std::cerr << "JsonUserSettings: Failed to save settings during destruction for "
                       << settings_file_path_.string() << std::endl;
        }
    }
}

bool JsonUserSettings::LoadSettings() {
    std::lock_guard<std::mutex> lock(mutex_); // Lock for reading file and modifying members

    try {
        // Check if the file exists
        if (!fs::exists(settings_file_path_)) {
            std::cout << "JsonUserSettings: Settings file not found: "
                      << settings_file_path_.string() << ". Creating default." << std::endl;
            // Create directory if it doesn't exist
            if (settings_file_path_.has_parent_path()) {
                 fs::create_directories(settings_file_path_.parent_path());
            }
             // Create an empty JSON object and save it to create the file
            settings_json_ = nlohmann::json::object();
            dirty_flag_ = true; // Mark dirty to ensure the file gets created
            return SaveSettings(); // Use SaveSettings logic to write the initial empty file
        }

        // File exists, try to read and parse
        std::ifstream file_stream(settings_file_path_);
        if (!file_stream.is_open()) {
             std::cerr << "JsonUserSettings: Failed to open settings file for reading: "
                       << settings_file_path_.string() << std::endl;
             // Keep existing potentially stale settings_json_ or default if construction just happened
             return false;
        }

        file_stream >> settings_json_;

        // Check if the root is an object (basic validation)
        if (!settings_json_.is_object()) {
            std::cerr << "JsonUserSettings: Invalid format in settings file (root is not an object): "
                      << settings_file_path_.string() << ". Resetting to empty." << std::endl;
            settings_json_ = nlohmann::json::object();
            dirty_flag_ = true; // Mark dirty to overwrite the invalid file
            return false; // Indicate load failure due to format
        }

        dirty_flag_ = false; // Successfully loaded, clear dirty flag
        std::cout << "JsonUserSettings: Settings loaded successfully from "
                  << settings_file_path_.string() << std::endl;
        return true;

    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JsonUserSettings: Failed to parse JSON settings file: "
                  << settings_file_path_.string() << ". Error: " << e.what() << ". Resetting to empty." << std::endl;
        settings_json_ = nlohmann::json::object();
        dirty_flag_ = true; // Mark dirty to overwrite the invalid file
        return false;
    } catch (const std::exception& e) {
        std::cerr << "JsonUserSettings: Filesystem or other error during load: "
                  << settings_file_path_.string() << ". Error: " << e.what() << std::endl;
        // Keep existing potentially stale settings_json_ or default
        return false;
    }
}

bool JsonUserSettings::SaveSettings() {
    std::lock_guard<std::mutex> lock(mutex_); // Lock for potentially writing file and modifying members

    if (!dirty_flag_) {
        // std::cout << "JsonUserSettings: No changes to save for " << settings_file_path_.string() << std::endl;
        return true; // Nothing to do
    }

    try {
         // Ensure directory exists before writing
         if (settings_file_path_.has_parent_path()) {
             fs::create_directories(settings_file_path_.parent_path());
         }

        std::ofstream file_stream(settings_file_path_);
        if (!file_stream.is_open()) {
            std::cerr << "JsonUserSettings: Failed to open settings file for writing: "
                      << settings_file_path_.string() << std::endl;
            return false;
        }

        // Write the JSON object with indentation for readability
        file_stream << settings_json_.dump(4); // Use dump(4) for pretty printing
        file_stream.close();

        if (!file_stream) { // Check for errors after closing
             std::cerr << "JsonUserSettings: Error occurred during writing or closing the settings file: "
                       << settings_file_path_.string() << std::endl;
            return false;
        }


        dirty_flag_ = false; // Successfully saved, clear dirty flag
        std::cout << "JsonUserSettings: Settings saved successfully to "
                  << settings_file_path_.string() << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "JsonUserSettings: Filesystem or other error during save: "
                  << settings_file_path_.string() << ". Error: " << e.what() << std::endl;
        return false;
    }
}

// Helper to get a setting node with type checking.
const nlohmann::json* JsonUserSettings::GetSettingNode(std::string_view key, nlohmann::json::value_t expected_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key_str(key); // nlohmann::json uses std::string keys

    auto it = settings_json_.find(key_str);
    if (it == settings_json_.end()) {
        return nullptr; // Key not found
    }

    if (it->type() != expected_type) {
         std::cerr << "JsonUserSettings: Type mismatch for key '" << key
                   << "'. Expected " << expected_type << ", got " << it->type() << std::endl;
        return nullptr; // Type mismatch
    }

    return &(*it); // Return pointer to the json node
}

std::optional<std::string> JsonUserSettings::GetString(std::string_view key) const {
    const nlohmann::json* node = GetSettingNode(key, nlohmann::json::value_t::string);
    if (!node) {
        return std::nullopt;
    }
    // No need to lock again, GetSettingNode already did
    return node->get<std::string>();
}

std::optional<int> JsonUserSettings::GetInt(std::string_view key) const {
    // JSON standard only has number type; we allow integer or floating point
    // but retrieve as int. Handle potential precision loss if needed elsewhere.
    const nlohmann::json* node = GetSettingNode(key, nlohmann::json::value_t::number_integer);
    // Allow number_unsigned or number_float as well? For now, strict integer.
    if (!node) {
         // Maybe try number_float too? For now, require integer.
         // node = GetSettingNode(key, nlohmann::json::value_t::number_float);
         // if (!node) return std::nullopt;
         return std::nullopt;
    }
     // No need to lock again
    // Add range checks if necessary?
    return node->get<int>();
}

std::optional<bool> JsonUserSettings::GetBool(std::string_view key) const {
    const nlohmann::json* node = GetSettingNode(key, nlohmann::json::value_t::boolean);
     if (!node) {
        return std::nullopt;
    }
    // No need to lock again
    return node->get<bool>();
}

void JsonUserSettings::SetString(std::string_view key, std::string_view value) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key_str(key);
    std::string value_str(value);

    // Check if value is actually changing to avoid unnecessary dirtying/saving
    auto it = settings_json_.find(key_str);
    if (it != settings_json_.end() && it->is_string() && it->get<std::string>() == value_str) {
        return; // Value is the same, do nothing
    }

    settings_json_[key_str] = value_str;
    dirty_flag_ = true;
}

void JsonUserSettings::SetInt(std::string_view key, int value) {
     std::lock_guard<std::mutex> lock(mutex_);
     std::string key_str(key);

     // Check if value is actually changing
     auto it = settings_json_.find(key_str);
     if (it != settings_json_.end() && it->is_number_integer() && it->get<int>() == value) {
         return; // Value is the same
     }

    settings_json_[key_str] = value;
    dirty_flag_ = true;
}

void JsonUserSettings::SetBool(std::string_view key, bool value) {
     std::lock_guard<std::mutex> lock(mutex_);
     std::string key_str(key);

      // Check if value is actually changing
     auto it = settings_json_.find(key_str);
     if (it != settings_json_.end() && it->is_boolean() && it->get<bool>() == value) {
         return; // Value is the same
     }

    settings_json_[key_str] = value;
    dirty_flag_ = true;
}

bool JsonUserSettings::HasKey(std::string_view key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return settings_json_.contains(std::string(key));
}

bool JsonUserSettings::RemoveKey(std::string_view key) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key_str(key);
    auto it = settings_json_.find(key_str);
    if (it != settings_json_.end()) {
        settings_json_.erase(it);
        dirty_flag_ = true;
        return true;
    }
    return false;
}

void JsonUserSettings::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!settings_json_.empty()) {
        settings_json_.clear(); // Clear the JSON object
        dirty_flag_ = true;
    }
}

} // namespace core
} // namespace game_launcher
