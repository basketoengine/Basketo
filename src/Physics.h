#pragma once
#include <SDL2/SDL.h>

class Physics {
public:
    // Static method: check AABB collision
    static bool checkCollision(const SDL_Rect& a, const SDL_Rect& b);
};
