#include "AssetManager.h"
#include <iostream>

AssetManager& AssetManager::getInstance() {
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

    if (textures.find(id) != textures.end()) {
        std::cout << "AssetManager Info: Texture with ID '" << id << "' already loaded." << std::endl;
        return true;
    }

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "AssetManager Error: Failed to load surface from '" << path << "'. IMG_Error: " << IMG_GetError() << std::endl;
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        std::cerr << "AssetManager Error: Failed to create texture from surface '" << path << "'. SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    textures[id] = texture;
    std::cout << "AssetManager Info: Loaded texture '" << id << "' from '" << path << "'" << std::endl;
    return true;
}

bool AssetManager::loadSound(const std::string& id, const std::string& path) {
    if (sounds.find(id) != sounds.end()) {
        std::cout << "AssetManager Info: Sound with ID '" << id << "' already loaded." << std::endl;
        return true;
    }

    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        std::cerr << "AssetManager Error: Failed to load sound '" << path << "'. Mix_Error: " << Mix_GetError() << std::endl;
        return false;
    }

    sounds[id] = chunk;
    std::cout << "AssetManager Info: Loaded sound '" << id << "' from '" << path << "'" << std::endl;
    return true;
}


bool AssetManager::loadFont(const std::string& id, const std::string& path, int fontSize) {

    std::string fontKey = id + "_" + std::to_string(fontSize);
    if (fonts.find(fontKey) != fonts.end()) {
        std::cout << "AssetManager Info: Font with ID '" << fontKey << "' already loaded." << std::endl;
        return true;
    }

    TTF_Font* font = TTF_OpenFont(path.c_str(), fontSize);
    if (!font) {
        std::cerr << "AssetManager Error: Failed to load font '" << path << "' with size " << fontSize << ". TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }

    fonts[fontKey] = font;
    std::cout << "AssetManager Info: Loaded font '" << fontKey << "' from '" << path << "'" << std::endl;
    return true;
}

SDL_Texture* AssetManager::getTexture(const std::string& id) const {
    auto it = textures.find(id);
    if (it != textures.end()) {
        return it->second;
    } else {
        std::cerr << "AssetManager Warning: Texture with ID '" << id << "' not found." << std::endl;
        return nullptr;
    }
}

Mix_Chunk* AssetManager::getSound(const std::string& id) const {
    auto it = sounds.find(id);
    if (it != sounds.end()) {
        return it->second;
    } else {
        std::cerr << "AssetManager Warning: Sound with ID '" << id << "' not found." << std::endl;
        return nullptr;
    }
}

TTF_Font* AssetManager::getFont(const std::string& id) const {
    auto it = fonts.find(id);
    if (it != fonts.end()) {
        return it->second;
    } else {
        std::cerr << "AssetManager Warning: Font with ID '" << id << "' not found." << std::endl;
        return nullptr;
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
    textures.clear();

    for (auto const& [id, sound] : sounds) {
        if (sound) {
            Mix_FreeChunk(sound);
            std::cout << "  - Freed sound chunk: " << id << std::endl;
        }
    }
    sounds.clear();

    for (auto const& [id, font] : fonts) {
        if (font) {
            TTF_CloseFont(font);
            std::cout << "  - Closed font: " << id << std::endl;
        }
    }
    fonts.clear();

    renderer = nullptr;
    std::cout << "AssetManager Info: Cleanup complete." << std::endl;
}
