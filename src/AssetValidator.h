#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

class AssetValidator {
public:
    static bool validateImage(const std::string& path);
    static bool validateAudio(const std::string& path);
    static bool validateScene(const std::string& path);
    static std::vector<std::string> validateDirectory(const std::string& path);
private:
    static bool checkFileExists(const std::string& path);
    static bool checkFileSize(const std::string& path, size_t maxSize);
};
