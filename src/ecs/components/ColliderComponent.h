#pragma once

#include <vector>
#include <string>
#include "../../../vendor/nlohmann/json.hpp" 

struct Vec2D {
    float x = 0.0f;
    float y = 0.0f;

    Vec2D() = default;
    Vec2D(float x_val, float y_val) : x(x_val), y(y_val) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Vec2D, x, y)
};

struct CollisionContact {
    Entity otherEntity = NO_ENTITY;
    Vec2D normal;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CollisionContact, otherEntity, normal)
};

struct ColliderComponent {

    float width = 0.0f;
    float height = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    std::vector<Vec2D> vertices;

    std::string type = "aabb";
    bool isTrigger = false; 

    std::vector<CollisionContact> contacts;

    ColliderComponent() = default;

    ColliderComponent(float w, float h, float offX = 0.0f, float offY = 0.0f, bool trigger = false)
        : width(w), height(h), offsetX(offX), offsetY(offY), type("aabb"), isTrigger(trigger) {}

    ColliderComponent(const std::vector<Vec2D>& verts, float offX = 0.0f, float offY = 0.0f, bool trigger = false)
        : vertices(verts), offsetX(offX), offsetY(offY), type("polygon"), isTrigger(trigger) {

    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ColliderComponent, width, height, offsetX, offsetY, vertices, type, isTrigger, contacts) // Added 'contacts'
};
