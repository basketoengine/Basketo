#pragma once

#include <string>
#include "../../vendor/nlohmann/json.hpp" // For JSON serialization

struct ScriptComponent {
    std::string scriptPath = "";
    // bool initialized = false; // Removed as per error analysis
};

// For nlohmann::json serialization
inline void to_json(nlohmann::json& j, const ScriptComponent& c) {
    j = nlohmann::json{{"scriptPath", c.scriptPath}};
}

inline void from_json(const nlohmann::json& j, ScriptComponent& c) {
    if (j.contains("scriptPath") && j.at("scriptPath").is_string()) {
        j.at("scriptPath").get_to(c.scriptPath);
    }
    // 'initialized' is deliberately not serialized/deserialized here;
    // it's a runtime state managed by the scene/script system.
}
