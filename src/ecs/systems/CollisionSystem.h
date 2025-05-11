#pragma once
#include "../System.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/RigidbodyComponent.h"
#include "../../Physics.h"
#include <SDL2/SDL.h>

class CollisionSystem : public System {
public:
    void update(ComponentManager* componentManager, float deltaTime) {
        for (auto const& entityA : entities) {
            auto& transformA = componentManager->getComponent<TransformComponent>(entityA);
            auto& rigidbodyA = componentManager->getComponent<RigidbodyComponent>(entityA);
            if (rigidbodyA.isStatic) continue;
            SDL_Rect rectA = {(int)transformA.x, (int)transformA.y, (int)transformA.width, (int)transformA.height};
            for (auto const& entityB : entities) {
                if (entityA == entityB) continue;
                auto& transformB = componentManager->getComponent<TransformComponent>(entityB);
                auto& rigidbodyB = componentManager->getComponent<RigidbodyComponent>(entityB);
                SDL_Rect rectB = {(int)transformB.x, (int)transformB.y, (int)transformB.width, (int)transformB.height};
                if (Physics::checkCollision(rectA, rectB)) {
                    auto& velocityA = componentManager->getComponent<VelocityComponent>(entityA);

                    float overlapY = (transformA.y + transformA.height) - transformB.y;

                    if (overlapY > 0) {
                        transformA.y -= overlapY;
                    }

                    if (velocityA.vy > 0) {
                         velocityA.vy = 0;
                    }

                    float overlapXRight = (transformA.x + transformA.width) - transformB.x;
                    float overlapXLeft = (transformB.x + transformB.width) - transformA.x;

                    if (overlapXRight > 0 && overlapXLeft > 0) { 
                        if (overlapXRight < overlapXLeft && velocityA.vx > 0) {
                            transformA.x -= overlapXRight;
                            velocityA.vx = 0;
                        } else if (overlapXLeft < overlapXRight && velocityA.vx < 0) { 
                            transformA.x += overlapXLeft;
                            velocityA.vx = 0;
                        }
                    }
                }
            }
        }
    }
};
