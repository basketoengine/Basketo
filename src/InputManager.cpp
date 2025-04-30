#include "InputManager.h"

InputManager& InputManager::getInstance() {
    static InputManager instance;
    return instance;
}

void InputManager::mapAction(const std::string& action, SDL_Scancode key) {
    actionMap[action] = key;
}

bool InputManager::isActionPressed(const std::string& action) const {
    auto it = actionMap.find(action);
    if (it != actionMap.end()) {
        return keyboardState && keyboardState[it->second];
    }
    return false;
}

void InputManager::update() {
    keyboardState = SDL_GetKeyboardState(nullptr);
}
