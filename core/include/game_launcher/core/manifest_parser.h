// core/include/game_launcher/core/manifest_parser.h
#pragma once

#include <string>
#include <vector>
#include <cstdint> // For uint64_t
#include "absl/status/statusor.h" // Include for StatusOr

namespace game_launcher::core {

// Represents a single file entry within the manifest
struct FileEntry {
    std::string path;         // Relative path within the install directory
    uint64_t size = 0;        // Uncompressed size in bytes
    std::string hash;         // Expected hash (e.g., SHA-256)
    std::string downloadUrl;  // URL to download the file from
    // Could add chunk info here later: std::vector<FileChunk> chunks;
};

// Represents the entire manifest structure
struct Manifest {
    std::string manifestVersion; // Version of the manifest format
    std::string gameVersion;     // Version of the game
    std::vector<FileEntry> files; // List of files in this version

    // Add methods for searching files, etc. if needed
};

// Function declaration for parsing (implementation will be in .cpp)
// Returns StatusOr containing the Manifest or an error status.
absl::StatusOr<Manifest> ParseManifestFromFile(const std::string& file_path);

} // namespace game_launcher::core
