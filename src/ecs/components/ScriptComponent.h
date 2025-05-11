#pragma once

#include <string>
#include "../../vendor/nlohmann/json.hpp"

struct ScriptComponent {
    std::string scriptPath = "";

    ScriptComponent() = default;
    explicit ScriptComponent(const std::string& path) : scriptPath(path) {}
};

inline void to_json(nlohmann::json& j, const ScriptComponent& c) {
    j = nlohmann::json{{"scriptPath", c.scriptPath}};
}

inline void from_json(const nlohmann::json& j, ScriptComponent& c) {
    if (j.contains("scriptPath") && j.at("scriptPath").is_string()) {
        j.at("scriptPath").get_to(c.scriptPath);
    }
}
