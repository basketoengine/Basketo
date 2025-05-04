#pragma once
#include <SDL2/SDL.h>

class Scene {
public:
    virtual ~Scene() = default;

    virtual void handleInput(SDL_Event& event) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
};
