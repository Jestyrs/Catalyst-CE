// core/src/json_user_settings.cpp
#include "game_launcher/core/json_user_settings.h"

#include <filesystem> // For create_directories
#include <fstream>    // For std::ifstream, std::ofstream
#include <iostream>   // For std::cerr (used in constructor/destructor)
#include <utility>    // For std::move

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h" // For absl::StrCat
#include "include/base/cef_logging.h" // Correct CEF logging include (full path)

// Include the AppSettings struct definition
#include "game_launcher/core/app_settings.h"

namespace game_launcher {
namespace core {

namespace fs = std::filesystem; // Alias for convenience

// --- Helper Functions ---

// Helper function to create error statuses
absl::Status FileError(const fs::path& path, const std::string& operation, const std::string& details = "") {
    std::string message = absl::StrCat("JsonUserSettings: Failed to ", operation, " settings file '", path.string(), "'");
    if (!details.empty()) {
        absl::StrAppend(&message, ". Details: ", details);
    }
    // Determine appropriate error code
    if (operation == "open" || operation == "read") return absl::UnavailableError(message);
    if (operation == "write" || operation == "create directory for") return absl::InternalError(message);
    if (operation == "parse" || operation == "deserialize") return absl::DataLossError(message);
    return absl::UnknownError(message);
}

absl::Status JsonUserSettings::EnsureDirectoryExists() {
    // No lock needed here, called internally by methods that already lock or can tolerate race condition
    try {
        if (settings_file_path_.has_parent_path() && !fs::exists(settings_file_path_.parent_path())) {
            fs::create_directories(settings_file_path_.parent_path());
            DLOG(INFO) << "JsonUserSettings: Created directory " << settings_file_path_.parent_path().string();
        }
        return absl::OkStatus();
    } catch (const std::exception& e) {
        return FileError(settings_file_path_, "create directory for", e.what());
    }
}

absl::StatusOr<nlohmann::json> JsonUserSettings::ReadJsonFromFile() {
    // No lock needed here, called internally by LoadSettings which locks.
    if (!fs::exists(settings_file_path_)) {
        return absl::NotFoundError(absl::StrCat("Settings file not found: ", settings_file_path_.string()));
    }

    std::ifstream file_stream(settings_file_path_);
    if (!file_stream.is_open()) {
        return FileError(settings_file_path_, "open");
    }

    try {
        nlohmann::json json_data;
        file_stream >> json_data;
        // Check stream state *after* attempting to read
        if (!file_stream.good() && !file_stream.eof()) { // Check for errors other than EOF
             return FileError(settings_file_path_, "read", "Stream error after parsing attempt");
        }
        return json_data;
    } catch (const nlohmann::json::parse_error& e) {
        return FileError(settings_file_path_, "parse", e.what());
    } catch (const std::exception& e) {
        // Catch potential filesystem errors or other general exceptions
        return FileError(settings_file_path_, "read", e.what());
    }
}

absl::Status JsonUserSettings::WriteJsonToFile(const nlohmann::json& json_data) {
    // No lock needed here, called internally by SaveSettings which locks.
    try {
        // Ensure directory exists before writing
        absl::Status dir_status = EnsureDirectoryExists();
        if (!dir_status.ok()) {
            return dir_status;
        }

        std::ofstream file_stream(settings_file_path_);
        if (!file_stream.is_open()) {
            return FileError(settings_file_path_, "open for writing");
        }

        // Write the JSON object with indentation for readability
        file_stream << json_data.dump(4); // Use dump(4) for pretty printing
        file_stream.flush(); // Explicitly flush the buffer

        if (!file_stream) { // Check stream state after writing/flushing
            file_stream.close(); // Attempt to close even if error occurred
            return FileError(settings_file_path_, "write", "Stream error after writing");
        }

        file_stream.close();

        if (!file_stream) { // Check stream state after closing
            return FileError(settings_file_path_, "close after writing", "Stream error after closing");
        }

        DLOG(INFO) << "JsonUserSettings: Wrote JSON to " << settings_file_path_.string();
        return absl::OkStatus();

    } catch (const std::exception& e) {
        // Catch potential filesystem errors during create_directories or ofstream construction/write
        return FileError(settings_file_path_, "write", e.what());
    }
}


// --- Constructor / Destructor ---

JsonUserSettings::JsonUserSettings(fs::path settings_file_path)
    : settings_file_path_(std::move(settings_file_path)),
      current_settings_(), // Initialize with default AppSettings
      settings_dirty_(false) { // Start clean
    // Eagerly load settings on construction. Handle potential load errors.
    absl::Status load_status = LoadSettings(); // LoadSettings locks the mutex
    if (!load_status.ok()) {
        LOG(ERROR) << "Failed to load user settings on initialization from '" << settings_file_path_.string() << "': " << load_status << ". Using default settings.";
        // Keep default current_settings_
        // If file didn't exist, LoadSettings already marked dirty_ = true.
        // If file was corrupt/invalid, mark dirty to ensure overwrite.
        if (!absl::IsNotFound(load_status)) {
             settings_dirty_ = true; // Mark dirty to force save of default settings later
        }
    }
}

JsonUserSettings::~JsonUserSettings() {
    // Attempt to save settings on destruction if they are dirty.
    if (settings_dirty_) {
        absl::Status save_status = SaveSettings(); // SaveSettings locks the mutex
        if (!save_status.ok()) {
            // Log error, but can't throw from destructor
            // Use std::cerr as LOG might not be safe in all destruction scenarios
            std::cerr << "JsonUserSettings: [Error] Failed to save settings during destruction for '"
                      << settings_file_path_.string() << "': " << save_status << std::endl;
        }
    }
}

// --- IUserSettings Implementation ---

absl::Status JsonUserSettings::LoadSettings() {
    std::lock_guard<std::mutex> lock(mutex_); // Lock for reading file and modifying members

    absl::StatusOr<nlohmann::json> json_data_status = ReadJsonFromFile();

    if (absl::IsNotFound(json_data_status.status())) {
        // File not found, use default settings and mark dirty to create it on save
        LOG(INFO) << "Settings file '" << settings_file_path_.string() << "' not found. Using default settings and scheduling creation.";
        current_settings_ = AppSettings{}; // Reset to default
        settings_dirty_ = true; // Mark dirty to ensure file creation on next save
        return absl::OkStatus(); // It's okay to start with defaults if file doesn't exist
    } else if (!json_data_status.ok()) {
        // Other error reading file (permission, disk error)
        LOG(ERROR) << "Failed to read settings file '" << settings_file_path_.string() << "': " << json_data_status.status();
        // Keep potentially stale current_settings_, don't mark dirty? Or reset to default?
        // Let's reset to default and mark dirty to attempt recovery on next save.
        current_settings_ = AppSettings{};
        settings_dirty_ = true;
        return json_data_status.status(); // Propagate the read error
    }

    // File read successfully, try to deserialize
    try {
        // Attempt to deserialize the entire JSON object into our struct
        // This relies on NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE in app_settings.h
        current_settings_ = json_data_status.value().get<AppSettings>();
        settings_dirty_ = false; // Successfully loaded, clear dirty flag
        DLOG(INFO) << "JsonUserSettings: Settings loaded successfully from " << settings_file_path_.string();
        return absl::OkStatus();
    } catch (const nlohmann::json::exception& e) {
        // Deserialization error (missing fields, wrong types, etc.)
        LOG(ERROR) << "Failed to parse/deserialize settings from file '" << settings_file_path_.string() << "'. Error: " << e.what() << ". Using default settings.";
        current_settings_ = AppSettings{}; // Reset to default on parse error
        settings_dirty_ = true; // Mark dirty to overwrite corrupted file on next save
        return FileError(settings_file_path_, "deserialize", e.what());
    }
}

absl::Status JsonUserSettings::SaveSettings() {
    std::lock_guard<std::mutex> lock(mutex_); // Lock for reading dirty flag and writing file

    if (!settings_dirty_) {
        DLOG(INFO) << "JsonUserSettings: No changes to save for " << settings_file_path_.string();
        return absl::OkStatus(); // Nothing to do
    }

    try {
        // Serialize the current settings struct to JSON
        // This relies on NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE in app_settings.h
        nlohmann::json json_data = current_settings_;

        // Write the JSON data to the file
        absl::Status write_status = WriteJsonToFile(json_data);
        if (!write_status.ok()) {
            LOG(ERROR) << "Failed to write settings to file '" << settings_file_path_.string() << "': " << write_status;
            return write_status; // Propagate the write error
        }

        settings_dirty_ = false; // Successfully saved, clear dirty flag
        LOG(INFO) << "JsonUserSettings: Settings saved successfully to " << settings_file_path_.string();
        return absl::OkStatus();

    } catch (const nlohmann::json::exception& e) {
        // Should generally not happen if AppSettings and its members are serializable
        LOG(ERROR) << "Failed to serialize settings to JSON for file '" << settings_file_path_.string() << "'. Error: " << e.what();
        return absl::InternalError(absl::StrCat("Failed to serialize settings: ", e.what()));
    } catch (const std::exception& e) {
        // Catch potential filesystem errors or other general exceptions during write
        LOG(ERROR) << "Unexpected error during settings save for '" << settings_file_path_.string() << "': " << e.what();
        return FileError(settings_file_path_, "write", e.what());
    }
}

AppSettings JsonUserSettings::GetAppSettings() const {
    std::lock_guard<std::mutex> lock(mutex_); // Lock for thread-safe read access
    return current_settings_; // Return a copy
}

absl::Status JsonUserSettings::SetAppSettings(const AppSettings& settings) {
    std::lock_guard<std::mutex> lock(mutex_); // Lock for thread-safe write access

    // Optional: Add validation logic for settings here if needed
    // if (!IsValid(settings)) {
    //     return absl::InvalidArgumentError("Provided settings are invalid.");
    // }

    // Check if settings actually changed to avoid unnecessary dirty flag/saves
    // This requires AppSettings to have an equality operator (==) defined.
    // If AppSettings doesn't have operator==, we assume it changed.
    // Add operator== to app_settings.h if you want this optimization.
    // if (current_settings_ != settings) {
         current_settings_ = settings;
         settings_dirty_ = true;
         DLOG(INFO) << "JsonUserSettings: Settings updated and marked dirty.";
    // } else {
    //     DLOG(INFO) << "JsonUserSettings: SetAppSettings called but settings were identical, not marking dirty.";
    // }
    return absl::OkStatus();
}


} // namespace core
} // namespace game_launcher
