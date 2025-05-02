\
#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <unordered_map>
#include <memory> // For std::unique_ptr if needed, though raw pointer for singleton is common

class AssetManager {
public:
    // Deleted copy constructor and assignment operator
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // Singleton access
    static AssetManager& getInstance();

    // Initialize the manager with the renderer
    void init(SDL_Renderer* renderer);

    // Load a texture from file and store it with an ID
    // Returns true on success, false on failure
    bool loadTexture(const std::string& id, const std::string& path);

    // Get a previously loaded texture by its ID
    // Returns nullptr if the texture is not found
    SDL_Texture* getTexture(const std::string& id) const;

    // Clean up all loaded assets
    void cleanup();

private:
    // Private constructor for singleton pattern
    AssetManager() = default;
    // Private destructor ensures cleanup is called explicitly or via singleton management
    ~AssetManager() = default; // Cleanup logic moved to cleanup() method

    SDL_Renderer* renderer = nullptr; // Renderer needed for texture creation
    std::unordered_map<std::string, SDL_Texture*> textures;
};
