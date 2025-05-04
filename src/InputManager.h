#pragma once
#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>

class InputManager {
public:
    static InputManager& getInstance();

    void mapAction(const std::string& action, SDL_Scancode key);
    bool isActionPressed(const std::string& action) const;
    void update();

private:
    InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    std::unordered_map<std::string, SDL_Scancode> actionMap;
    const Uint8* keyboardState = nullptr;
};
