#include "AssetValidator.h"
#include <iostream>

bool AssetValidator::validateImage(const std::string& path) {
    if (!checkFileExists(path)) return false;
    
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) return false;
    SDL_FreeSurface(surface);
    
    return true;
}

bool AssetValidator::validateAudio(const std::string& path) {
    if (!checkFileExists(path)) return false;
    
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) return false;
    Mix_FreeChunk(chunk);
    
    return true;
}

bool AssetValidator::validateScene(const std::string& path) {
    if (!checkFileExists(path)) return false;
    
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    file.close();
    return true;
}

std::vector<std::string> AssetValidator::validateDirectory(const std::string& path) {
    std::vector<std::string> invalidAssets;
    
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        std::string ext = entry.path().extension().string();
        
        if (ext == ".png" || ext == ".jpg") {
            if (!validateImage(entry.path().string()))
                invalidAssets.push_back(entry.path().string());
        }
        else if (ext == ".wav" || ext == ".mp3") {
            if (!validateAudio(entry.path().string()))
                invalidAssets.push_back(entry.path().string());
        }
        else if (ext == ".scene") {
            if (!validateScene(entry.path().string()))
                invalidAssets.push_back(entry.path().string());
        }
    }
    
    return invalidAssets;
}

bool AssetValidator::checkFileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool AssetValidator::checkFileSize(const std::string& path, size_t maxSize) {
    return std::filesystem::file_size(path) <= maxSize;
}
