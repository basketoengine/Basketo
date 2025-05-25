#pragma once
#include "../System.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/RigidbodyComponent.h"
#include "../../Physics.h"
#include "../../spatial/Quadtree.h" 
#include <SDL2/SDL.h>
#include <memory>
#include <vector> 

class CollisionSystem : public System {
public:
    std::unique_ptr<Quadtree> quadtree;

    CollisionSystem(float worldWidth = 2000.0f, float worldHeight = 1500.0f) { 
        SDL_Rect worldBounds = {0, 0, static_cast<int>(worldWidth), static_cast<int>(worldHeight)};
        quadtree = std::make_unique<Quadtree>(0, worldBounds); 
    }

    void update(ComponentManager* componentManager, float deltaTime) {
        if (!quadtree) return; 

        quadtree->clear();

        for (auto const& entity : entities) {
            if (componentManager->hasComponent<TransformComponent>(entity) && 
                componentManager->hasComponent<RigidbodyComponent>(entity)) {
                auto& transform = componentManager->getComponent<TransformComponent>(entity);
                quadtree->insert(entity, transform);
            }
        }

        for (auto const& entityA : entities) {
            if (!componentManager->hasComponent<TransformComponent>(entityA) || 
                !componentManager->hasComponent<RigidbodyComponent>(entityA)) {
                continue; 
            }

            auto& transformA = componentManager->getComponent<TransformComponent>(entityA);
            auto& rigidbodyA = componentManager->getComponent<RigidbodyComponent>(entityA);

            if (rigidbodyA.isStatic) continue; 

            SDL_Rect rectA = {(int)transformA.x, (int)transformA.y, (int)transformA.width, (int)transformA.height};
            
            std::vector<Entity> potentialColliders = quadtree->query(transformA);

            for (auto const& entityB : potentialColliders) {
                if (entityA == entityB) continue;

                if (!componentManager->hasComponent<TransformComponent>(entityB) || 
                    !componentManager->hasComponent<RigidbodyComponent>(entityB)) {
                    continue;
                }

                auto& transformB = componentManager->getComponent<TransformComponent>(entityB);
                auto& rigidbodyB = componentManager->getComponent<RigidbodyComponent>(entityB);
                SDL_Rect rectB = {(int)transformB.x, (int)transformB.y, (int)transformB.width, (int)transformB.height};

                if (Physics::checkCollision(rectA, rectB)) {
                    if (!componentManager->hasComponent<VelocityComponent>(entityA)) {
                        continue;
                    }
                    auto& velocityA = componentManager->getComponent<VelocityComponent>(entityA);

                    // Basic collision response: resolve overlap and stop movement towards collision
                    // Y-axis resolution
                    float overlapY = (transformA.y + transformA.height) - transformB.y;
                    float overlapY_BBelowA = (transformB.y + transformB.height) - transformA.y; 

                    if (rectA.y < rectB.y + rectB.h && rectA.y + rectA.h > rectB.y) {
                        if (velocityA.vy > 0 && overlapY > 0 && overlapY < transformB.height) { 
                            transformA.y -= overlapY;
                            velocityA.vy = 0;
                        } else if (velocityA.vy < 0 && overlapY_BBelowA > 0 && overlapY_BBelowA < transformB.height) { 
                            transformA.y += overlapY_BBelowA;
                            velocityA.vy = 0;
                        }
                    }
                    
                    // Re-calculate rectA after Y correction for X-axis resolution
                    rectA.y = (int)transformA.y;

                    // X-axis resolution
                    float overlapXRight = (transformA.x + transformA.width) - transformB.x; // A is to the left of B, A hits B's left side
                    float overlapXLeft = (transformB.x + transformB.width) - transformA.x; // A is to the right of B, A hits B's right side

                    if (rectA.x < rectB.x + rectB.w && rectA.x + rectA.w > rectB.x) { // Check for actual X overlap
                        if (velocityA.vx > 0 && overlapXRight > 0 && overlapXRight < transformB.width) {
                            transformA.x -= overlapXRight;
                            velocityA.vx = 0;
                        } else if (velocityA.vx < 0 && overlapXLeft > 0 && overlapXLeft < transformB.width) { 
                            transformA.x += overlapXLeft;
                            velocityA.vx = 0;
                        }
                    }
                }
            }
        }
    }
};
