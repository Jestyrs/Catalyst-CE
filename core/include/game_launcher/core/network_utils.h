#ifndef GAME_LAUNCHER_CORE_NETWORK_UTILS_H_
#define GAME_LAUNCHER_CORE_NETWORK_UTILS_H_

#include <string>
#include <absl/status/statusor.h>
#include <filesystem>

namespace game_launcher {
namespace core {

/**
 * @brief Downloads the content of a given URL as a string.
 *
 * IMPORTANT: curl_global_init() must be called once before using this function,
 * and curl_global_cleanup() must be called once before the application exits.
 *
 * @param url The URL to download from.
 * @return absl::StatusOr<std::string> Containing the downloaded content on success,
 *         or an error Status on failure.
 */
absl::StatusOr<std::string> DownloadString(const std::string& url);

/**
 * @brief Downloads the content of a given URL to a specified file path.
 *
 * IMPORTANT: curl_global_init() must be called once before using this function,
 * and curl_global_cleanup() must be called once before the application exits.
 *
 * @param url The URL to download from.
 * @param destination_path The full path where the downloaded file should be saved.
 * @return absl::Status OkStatus on success, or an error Status on failure.
 */
absl::Status DownloadFile(const std::string& url, const std::filesystem::path& destination_path);

} // namespace core
} // namespace game_launcher

#endif // GAME_LAUNCHER_CORE_NETWORK_UTILS_H_
