#pragma once

#include "../../../vendor/nlohmann/json.hpp"

struct VelocityComponent {
    float vx = 0.0f;
    float vy = 0.0f;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VelocityComponent, vx, vy)
