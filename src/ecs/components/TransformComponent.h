#pragma once
#include "../../vendor/nlohmann/json.hpp"

struct TransformComponent {
    float x = 0.0f;
    float y = 0.0f;
    float width = 32.0f;
    float height = 32.0f;
    // Add scale, rotation as floats if needed
};

// Serialization functions for TransformComponent
inline void to_json(nlohmann::json& j, const TransformComponent& c) {
    j = nlohmann::json{
        {"x", c.x},
        {"y", c.y},
        {"width", c.width},
        {"height", c.height}
        // Add scale, rotation if saving them
    };
}

inline void from_json(const nlohmann::json& j, TransformComponent& c) {
    c.x = j.value("x", 0.0f);
    c.y = j.value("y", 0.0f);
    c.width = j.value("width", 32.0f);
    c.height = j.value("height", 32.0f);
    // Add scale, rotation loading if needed
}
