#include "game_launcher/core/network_utils.h"

#include <curl/curl.h> // libcurl header
#include <sstream>
#include <memory> // For std::unique_ptr
#include <absl/status/status.h> // For absl::OkStatus and error statuses
#include <absl/strings/str_cat.h> // For StrCat
#include <fstream> // Added for std::ofstream
#include <filesystem> // Added for std::filesystem::path

namespace game_launcher {
namespace core {

// Callback function for libcurl to write received data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    static_cast<std::stringstream*>(userp)->write(static_cast<char*>(contents), real_size);
    return real_size;
}

// Helper struct for curl_easy_cleanup
struct CurlEasyCleanup {
    void operator()(CURL* handle) {
        if (handle) {
            curl_easy_cleanup(handle);
        }
    }
};
using CurlEasyHandle = std::unique_ptr<CURL, CurlEasyCleanup>;

// libcurl write callback function for writing to a file
static size_t WriteToFileCallback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

absl::StatusOr<std::string> DownloadString(const std::string& url) {
    CurlEasyHandle curl = CurlEasyHandle(curl_easy_init());
    std::stringstream buffer;
    long http_code = 0;

    if (!curl) {
        return absl::InternalError("Failed to initialize curl easy handle.");
    }

    // Set URL
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    // Set write callback function and data buffer
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &buffer);
    // Follow redirects
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
    // Set a reasonable timeout
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L); // 30 seconds timeout
    // Fail on HTTP errors (4xx or 5xx)
    curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 1L);
    // Enable verbose output for debugging (optional)
    // curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl.get());

    // Check for errors
    if (res != CURLE_OK) {
        return absl::InternalError(absl::StrCat("curl_easy_perform() failed: ", curl_easy_strerror(res)));
    }

    // Check HTTP response code (optional, as FAILONERROR handles 4xx/5xx)
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code >= 400) {
        // This might be redundant if FAILONERROR is set, but good for clarity
         return absl::InternalError(absl::StrCat("HTTP error ", http_code, " for URL: ", url));
    }

    return buffer.str();
}

absl::Status DownloadFile(const std::string& url, const std::filesystem::path& destination_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return absl::InternalError("Failed to initialize libcurl handle.");
    }

    // Using std::unique_ptr for automatic cleanup of the CURL handle
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl_handle(curl, curl_easy_cleanup);

    // Open the destination file for writing in binary mode
    // Using C-style FILE* as it's often easier with libcurl's C API
    FILE* fp = nullptr;
#ifdef _WIN32
    // Use _wfopen on Windows for wide character path support
    errno_t err = _wfopen_s(&fp, destination_path.c_str(), L"wb");
    if (err != 0 || fp == nullptr) {
        return absl::InternalError(absl::StrCat("Failed to open file for writing: ", destination_path.string(), ", error: ", err));
    }
#else
    // Standard fopen on other platforms
    fp = fopen(destination_path.c_str(), "wb");
    if (fp == nullptr) {
        return absl::InternalError(absl::StrCat("Failed to open file for writing: ", destination_path.string()));
    }
#endif

    // Ensure the file is closed when the function exits
    // Using a custom deleter with unique_ptr for FILE*
    std::unique_ptr<FILE, decltype(&fclose)> file_handle(fp, fclose);

    // Set libcurl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);    // Fail on HTTP errors >= 400

    // TODO: Add progress callback later
    // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    // curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
    // curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, /* some progress struct */);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK) {
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        // Explicitly close the file handle *before* potentially deleting the file
        file_handle.reset(); 
        // Attempt to delete partially downloaded file on error
        std::error_code ec;
        std::filesystem::remove(destination_path, ec);
        // Return specific errors if possible
        if (res == CURLE_HTTP_RETURNED_ERROR) {
             return absl::InternalError(absl::StrCat("HTTP error during download from ", url, ": ", response_code));
        }
         return absl::InternalError(absl::StrCat("libcurl error during download from ", url, ": ", curl_easy_strerror(res)));
    }

    return absl::OkStatus();
}

} // namespace core
} // namespace game_launcher
