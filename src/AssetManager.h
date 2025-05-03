#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h> 
#include <SDL2/SDL_ttf.h> 
#include <string>
#include <unordered_map>
#include <memory> 

class AssetManager {
public:
    
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // Singleton access
    static AssetManager& getInstance();

    void init(SDL_Renderer* renderer);

    bool loadTexture(const std::string& id, const std::string& path);

    bool loadSound(const std::string& id, const std::string& path);

    bool loadFont(const std::string& id, const std::string& path, int fontSize);

    SDL_Texture* getTexture(const std::string& id) const;

    Mix_Chunk* getSound(const std::string& id) const;

    TTF_Font* getFont(const std::string& id) const;

    void cleanup();

private:
    // Private constructor for singleton pattern
    AssetManager() = default;
    ~AssetManager() = default; 

    SDL_Renderer* renderer = nullptr;
    std::unordered_map<std::string, SDL_Texture*> textures;
    std::unordered_map<std::string, Mix_Chunk*> sounds; 
    std::unordered_map<std::string, TTF_Font*> fonts;
};
