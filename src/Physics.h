#pragma once
#include <SDL2/SDL.h>

class TransformComponent;

class Physics {
public:
    static bool checkCollision(const SDL_Rect& a, const SDL_Rect& b);
    static void confineToWorldBounds(TransformComponent& transform, const SDL_Rect& worldBounds);
};
