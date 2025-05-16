#pragma once

#include <string>
#include <SDL2/SDL.h>
#include "../../vendor/nlohmann/json.hpp"

struct SpriteComponent {
    std::string textureId;
    SDL_Rect srcRect = {0,0,0,0};
    bool useSrcRect = false; 
    int layer = 0;
    bool isFixed = false; // For UI elements that shouldn't move with camera
    SDL_RendererFlip flip = SDL_FLIP_NONE; // Add flip member

    SpriteComponent() = default;
    SpriteComponent(const std::string& id) : textureId(id) {}
    SpriteComponent(const std::string& id, SDL_Rect sr, int l = 0, bool fixed = false, SDL_RendererFlip f = SDL_FLIP_NONE)
        : textureId(id), srcRect(sr), useSrcRect(true), layer(l), isFixed(fixed), flip(f) {}
};

inline void to_json(nlohmann::json& j, const SpriteComponent& c) {
    j = nlohmann::json{
        {"textureId", c.textureId},
        {"srcRect", {{"x", c.srcRect.x}, {"y", c.srcRect.y}, {"w", c.srcRect.w}, {"h", c.srcRect.h}}},
        {"layer", c.layer},
        {"isFixed", c.isFixed},
        {"flip", static_cast<int>(c.flip)} // Serialize flip
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
    c.flip = static_cast<SDL_RendererFlip>(j.value("flip", static_cast<int>(SDL_FLIP_NONE))); // Deserialize flip
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SpriteComponent, textureId, srcRect.x, srcRect.y, srcRect.w, srcRect.h, useSrcRect, layer, isFixed, flip); // Add flip to serialization
