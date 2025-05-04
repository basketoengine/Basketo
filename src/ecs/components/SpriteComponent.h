#pragma once

#include <string>
#include <SDL2/SDL.h>
#include "../../vendor/nlohmann/json.hpp"

struct SpriteComponent {
    std::string textureId;
    SDL_Rect srcRect = {0, 0, 0, 0};
    bool useSrcRect = false;
    int layer = 0;
    bool isFixed = false;

    SpriteComponent() : textureId(""), useSrcRect(false), layer(0), isFixed(false) {}
    SpriteComponent(const std::string& id) : textureId(id), useSrcRect(false), layer(0), isFixed(false) {}
    SpriteComponent(const std::string& id, int x, int y, int w, int h)
        : textureId(id), srcRect{x, y, w, h}, useSrcRect(true), layer(0), isFixed(false) {}
};

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
