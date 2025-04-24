// core/src/manifest_parser.cpp
#include "game_launcher/core/manifest_parser.h"

#include <fstream> // For file streaming
#include <nlohmann/json.hpp> // For JSON parsing
#include "absl/status/status.h" // For returning status
#include "absl/status/statusor.h" // For returning status or value
#include "absl/strings/str_format.h" // For formatting error messages

using json = nlohmann::json;

namespace game_launcher::core {

// Helper function to safely get a value from JSON or return error
// (Could be enhanced to check types)
template <typename T>
absl::StatusOr<T> GetJsonValue(const json& j, const std::string& key) {
    if (!j.contains(key)) {
        return absl::NotFoundError(absl::StrFormat("Manifest missing required key: '%s'", key));
    }
    try {
        return j.at(key).get<T>();
    } catch (const json::exception& e) {
        return absl::InvalidArgumentError(absl::StrFormat("Error reading key '%s': %s", key, e.what()));
    }
}

// Modified function declaration to return StatusOr
absl::StatusOr<Manifest> ParseManifestFromFile(const std::string& file_path) {
    Manifest manifest;
    std::ifstream file_stream(file_path);

    if (!file_stream.is_open()) {
        return absl::NotFoundError(absl::StrFormat("Could not open manifest file: %s", file_path));
    }

    json manifest_json;
    try {
        file_stream >> manifest_json;
    } catch (const json::parse_error& e) {
        return absl::InvalidArgumentError(absl::StrFormat("Failed to parse manifest JSON '%s': %s", file_path, e.what()));
    }

    // Parse top-level fields
    auto manifest_version_or = GetJsonValue<std::string>(manifest_json, "manifestVersion");
    if (!manifest_version_or.ok()) return manifest_version_or.status();
    manifest.manifestVersion = *manifest_version_or;

    auto game_version_or = GetJsonValue<std::string>(manifest_json, "gameVersion");
    if (!game_version_or.ok()) return game_version_or.status();
    manifest.gameVersion = *game_version_or;

    // Parse files array
    if (!manifest_json.contains("files") || !manifest_json["files"].is_array()) {
        return absl::InvalidArgumentError("Manifest missing or invalid 'files' array.");
    }

    for (const auto& file_entry_json : manifest_json["files"]) {
        if (!file_entry_json.is_object()) {
            return absl::InvalidArgumentError("Invalid entry in 'files' array: not an object.");
        }

        FileEntry entry;
        auto path_or = GetJsonValue<std::string>(file_entry_json, "path");
        if (!path_or.ok()) return path_or.status();
        entry.path = *path_or;

        // Use uint64_t for size
        auto size_or = GetJsonValue<uint64_t>(file_entry_json, "size"); 
        if (!size_or.ok()) return size_or.status();
        entry.size = *size_or;

        auto hash_or = GetJsonValue<std::string>(file_entry_json, "hash");
        if (!hash_or.ok()) return hash_or.status();
        entry.hash = *hash_or;

        auto url_or = GetJsonValue<std::string>(file_entry_json, "downloadUrl");
        if (!url_or.ok()) return url_or.status();
        entry.downloadUrl = *url_or;

        manifest.files.push_back(std::move(entry));
    }

    return manifest;
}

} // namespace game_launcher::core
