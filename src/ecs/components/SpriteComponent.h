#pragma once

#include <string>
#include <SDL2/SDL.h> // Include for SDL_Rect if needed for source rect

struct SpriteComponent {
    std::string textureId; // ID used to lookup texture in AssetManager
    SDL_Rect srcRect;      // Source rectangle within the texture atlas (optional)
    bool useSrcRect = false; // Flag to indicate if srcRect should be used

    // Default constructor
    SpriteComponent() : textureId(""), useSrcRect(false) {} // Add default constructor
    
    // Constructor for simple texture ID
    SpriteComponent(const std::string& id) : textureId(id), useSrcRect(false) {}

    // Constructor for texture ID and source rectangle
    SpriteComponent(const std::string& id, int x, int y, int w, int h)
        : textureId(id), srcRect{x, y, w, h}, useSrcRect(true) {}
};
