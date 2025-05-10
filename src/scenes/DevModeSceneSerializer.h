\
#pragma once

#include <string>

// Forward declaration
class DevModeScene;

void saveDevModeScene(DevModeScene& scene, const std::string& filepath);
void loadDevModeScene(DevModeScene& scene, const std::string& filepath);
