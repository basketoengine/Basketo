#include "Entity.h"

Entity::Entity(int x, int y, int w, int h) {
    rect = { x, y, w, h };
}

void Entity::update(float deltaTime) {
    rect.x += static_cast<int>(velocityX * deltaTime);
    rect.y += static_cast<int>(velocityY * deltaTime);
}

void Entity::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255); 
    SDL_RenderFillRect(renderer, &rect);
}

void Entity::setVelocity(float vx, float vy) {
    velocityX = vx;
    velocityY = vy;
}

void Entity::move(float dx, float dy) {
    rect.x += static_cast<int>(dx);
    rect.y += static_cast<int>(dy);
}
