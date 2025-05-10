#ifndef COLLIDERCOMPONENT_H
#define COLLIDERCOMPONENT_H

#include "../../../vendor/nlohmann/json.hpp"
#include <vector>

struct ColliderVertex {
    float x = 0.0f;
    float y = 0.0f;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ColliderVertex, x, y)
};

struct ColliderComponent {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float width = 32.0f;  // Default width (for box collider)
    float height = 32.0f; // Default height (for box collider)
    bool isTrigger = false;
    std::vector<ColliderVertex> vertices; // Polygon vertices (local space)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ColliderComponent, offsetX, offsetY, width, height, isTrigger, vertices)
};

#endif // COLLIDERCOMPONENT_H
