\
#include "AssetManager.h"
#include <iostream> // For error reporting

// Initialize singleton instance (implementation detail)
// AssetManager* AssetManager::instance = nullptr; // If using pointer approach

AssetManager& AssetManager::getInstance() {
    // Static instance created on first call and guaranteed to be destroyed.
    static AssetManager instance;
    return instance;
}

void AssetManager::init(SDL_Renderer* ren) {
    renderer = ren;
    if (!renderer) {
        std::cerr << "AssetManager Error: Initialized with null renderer!" << std::endl;
    }
}

bool AssetManager::loadTexture(const std::string& id, const std::string& path) {
    if (!renderer) {
        std::cerr << "AssetManager Error: Cannot load texture, renderer is not initialized." << std::endl;
        return false;
    }

    // Check if texture already exists
    if (textures.find(id) != textures.end()) {
        std::cout << "AssetManager Info: Texture with ID '" << id << "' already loaded." << std::endl;
        return true; // Already loaded, consider it a success
    }

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "AssetManager Error: Failed to load surface from '" << path << "'. IMG_Error: " << IMG_GetError() << std::endl;
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface); // Free the surface regardless of texture creation success

    if (!texture) {
        std::cerr << "AssetManager Error: Failed to create texture from surface '" << path << "'. SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Optional: Set blend mode or other texture properties if needed globally
    // SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    textures[id] = texture; // Store the texture
    std::cout << "AssetManager Info: Loaded texture '" << id << "' from '" << path << "'" << std::endl;
    return true;
}

SDL_Texture* AssetManager::getTexture(const std::string& id) const {
    auto it = textures.find(id);
    if (it != textures.end()) {
        return it->second; // Return the found texture
    } else {
        std::cerr << "AssetManager Warning: Texture with ID '" << id << "' not found." << std::endl;
        return nullptr; // Texture not found
    }
}

void AssetManager::cleanup() {
    std::cout << "AssetManager Info: Cleaning up assets..." << std::endl;
    for (auto const& [id, texture] : textures) {
        if (texture) {
            SDL_DestroyTexture(texture);
            std::cout << "  - Destroyed texture: " << id << std::endl;
        }
    }
    textures.clear(); // Clear the map
    renderer = nullptr; // Reset renderer pointer
    std::cout << "AssetManager Info: Cleanup complete." << std::endl;
}
