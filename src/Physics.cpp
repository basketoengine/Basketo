#include "Physics.h"
#include "ecs/components/TransformComponent.h" 

bool Physics::checkCollision(const SDL_Rect& a, const SDL_Rect& b) {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

void Physics::confineToWorldBounds(TransformComponent& transform, const SDL_Rect& worldBounds) {
    if (transform.x < worldBounds.x) {
        transform.x = static_cast<float>(worldBounds.x);
    }
    if (transform.y < worldBounds.y) {
        transform.y = static_cast<float>(worldBounds.y);
    }
    if (transform.x + transform.width > worldBounds.x + worldBounds.w) {
        transform.x = static_cast<float>(worldBounds.x + worldBounds.w) - transform.width;
    }
    if (transform.y + transform.height > worldBounds.y + worldBounds.h) {
        transform.y = static_cast<float>(worldBounds.y + worldBounds.h) - transform.height;
    }
}
