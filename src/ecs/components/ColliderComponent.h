#ifndef COLLIDERCOMPONENT_H
#define COLLIDERCOMPONENT_H

#include "../../../vendor/nlohmann/json.hpp"

struct ColliderComponent {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float width = 32.0f;  // Default width
    float height = 32.0f; // Default height
    bool isTrigger = false;

    // Optional: Add a tag or layer for collision filtering later
    // int layer = 0; 
    // std::string tag = "default";

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ColliderComponent, offsetX, offsetY, width, height, isTrigger)
};

#endif // COLLIDERCOMPONENT_H
