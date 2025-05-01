#pragma once
#include <SDL2/SDL.h>

class Entity {
public:
    Entity(int x, int y, int w, int h);

    void update(float deltaTime);
    void render(SDL_Renderer* renderer);

    void setVelocity(float vx, float vy);
    void move(float dx, float dy);

private:
    SDL_Rect rect;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
};
