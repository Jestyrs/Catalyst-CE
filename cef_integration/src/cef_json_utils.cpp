#include "game_launcher/cef_integration/cef_json_utils.h"
#include "include/base/cef_logging.h"

namespace game_launcher::cef_integration {

CefRefPtr<CefDictionaryValue> JsonToCefDictionary(const nlohmann::json& json_obj) {
    if (!json_obj.is_object()) {
        LOG(ERROR) << "Input nlohmann::json is not an object.";
        return nullptr;
    }

    CefRefPtr<CefDictionaryValue> dict = CefDictionaryValue::Create();

    for (auto& [key, val] : json_obj.items()) {
        if (val.is_string()) {
            dict->SetString(key, val.get<std::string>());
        } else if (val.is_number()) { // Includes integers and floats
            dict->SetDouble(key, val.get<double>());
        } else if (val.is_boolean()) {
            dict->SetBool(key, val.get<bool>());
        } else if (val.is_null()) {
            dict->SetNull(key);
        } else {
            // For simplicity, skipping nested objects/arrays for now
            // LOG(WARNING) << "Skipping unsupported JSON type for key: " << key;
        }
    }

    return dict;
}

} // namespace game_launcher::cef_integration
