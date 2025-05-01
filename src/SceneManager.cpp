#include "SceneManager.h"

SceneManager& SceneManager::getInstance() {
    static SceneManager instance;
    return instance;
}

void SceneManager::changeScene(std::unique_ptr<Scene> newScene) {
    activeScene = std::move(newScene);
}

Scene* SceneManager::getActiveScene() {
    return activeScene.get();
}
