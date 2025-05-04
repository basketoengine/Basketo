#pragma once

#include <string>
#include <SDL2/SDL.h> // Include for SDL_Rect if needed for source rect
#include "../../vendor/nlohmann/json.hpp" // Corrected spelling

struct SpriteComponent {
    std::string textureId; // ID used to lookup texture in AssetManager
    SDL_Rect srcRect = {0, 0, 0, 0}; // Initialize source rectangle
    bool useSrcRect = false; // Flag to indicate if srcRect should be used
    int layer = 0; // Layer for rendering order
    bool isFixed = false; // For UI elements, etc.

    // Default constructor
    SpriteComponent() : textureId(""), useSrcRect(false), layer(0), isFixed(false) {}

    // Constructor for simple texture ID
    SpriteComponent(const std::string& id) : textureId(id), useSrcRect(false), layer(0), isFixed(false) {}

    // Constructor for texture ID and source rectangle
    SpriteComponent(const std::string& id, int x, int y, int w, int h)
        : textureId(id), srcRect{x, y, w, h}, useSrcRect(true), layer(0), isFixed(false) {}
};

// Serialization functions for SpriteComponent
inline void to_json(nlohmann::json& j, const SpriteComponent& c) {
    j = nlohmann::json{
        {"textureId", c.textureId},
        {"srcRect", {{"x", c.srcRect.x}, {"y", c.srcRect.y}, {"w", c.srcRect.w}, {"h", c.srcRect.h}}},
        {"layer", c.layer},
        {"isFixed", c.isFixed}
    };
}

inline void from_json(const nlohmann::json& j, SpriteComponent& c) {
    j.at("textureId").get_to(c.textureId);
    c.srcRect.x = j.at("srcRect").value("x", 0);
    c.srcRect.y = j.at("srcRect").value("y", 0);
    c.srcRect.w = j.at("srcRect").value("w", 0);
    c.srcRect.h = j.at("srcRect").value("h", 0);
    j.at("layer").get_to(c.layer);
    j.at("isFixed").get_to(c.isFixed);
}
