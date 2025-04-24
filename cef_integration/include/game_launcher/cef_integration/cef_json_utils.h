#pragma once

#include "include/cef_values.h"
#include "nlohmann/json.hpp"

namespace game_launcher::cef_integration {

/**
 * @brief Converts a nlohmann::json object to a CefDictionaryValue.
 *
 * Handles basic types: string, number (as double), boolean, and null.
 * Does not handle nested objects or arrays recursively for simplicity.
 * @param json_obj The nlohmann::json object to convert (must be an object type).
 * @return A CefRefPtr<CefDictionaryValue> representing the JSON object, or nullptr on error.
 */
CefRefPtr<CefDictionaryValue> JsonToCefDictionary(const nlohmann::json& json_obj);

} // namespace game_launcher::cef_integration
