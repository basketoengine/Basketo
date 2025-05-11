\
#pragma once

#include <string>

class DevModeScene;

void saveDevModeScene(DevModeScene& scene, const std::string& filepath);
bool loadDevModeScene(DevModeScene& scene, const std::string& filepath);
