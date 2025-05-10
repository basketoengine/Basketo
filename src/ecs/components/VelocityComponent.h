#pragma once

#include "../../../vendor/nlohmann/json.hpp"

struct VelocityComponent {
    float vx = 0.0f;
    float vy = 0.0f;
};

inline void to_json(nlohmann::json& j, const VelocityComponent& c) {
    j = nlohmann::json{{"vx", c.vx}, {"vy", c.vy}};
}

inline void from_json(const nlohmann::json& j, VelocityComponent& c) {
    c.vx = j.value("vx", 0.0f);
    c.vy = j.value("vy", 0.0f);
}
