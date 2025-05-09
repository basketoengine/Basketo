#include "FileUtils.h"
#include <filesystem>
#include <iostream>

std::string getFilenameFromPath(const std::string& path) {
    try {
        return std::filesystem::path(path).filename().string();
    } catch (const std::exception& e) {
        std::cerr << "Error getting filename: " << e.what() << std::endl;
        return "";
    }
}

std::string getFilenameWithoutExtension(const std::string& filename) {
    try {
        return std::filesystem::path(filename).stem().string();
    } catch (const std::exception& e) {
        std::cerr << "Error getting stem: " << e.what() << std::endl;
        return "";
    }
}
