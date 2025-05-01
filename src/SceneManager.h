#pragma once
#include "Scene.h"
#include <memory>

class SceneManager {
public:
    static SceneManager& getInstance();

    void changeScene(std::unique_ptr<Scene> newScene);
    Scene* getActiveScene();

private:
    SceneManager() = default;
    std::unique_ptr<Scene> activeScene;
};
