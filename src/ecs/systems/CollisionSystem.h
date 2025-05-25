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
#include <iostream> 

class CollisionSystem : public System {
public:
    std::unique_ptr<Quadtree> quadtree;
    int entitiesInsertedIntoQuadtree = 0;

    CollisionSystem(float worldWidth = 2000.0f, float worldHeight = 1500.0f) { 
        std::cout << "[CollisionSystem] Constructor called. World: " << worldWidth << "x" << worldHeight << std::endl;
        SDL_Rect worldBounds = {0, 0, static_cast<int>(worldWidth), static_cast<int>(worldHeight)};
        quadtree = std::make_unique<Quadtree>(0, worldBounds); 
    }

    ~CollisionSystem() {
        std::cout << "[CollisionSystem] Destructor called." << std::endl;
    }

    void update(ComponentManager* componentManager, float deltaTime) {
        std::cout << "[CollisionSystem] Update called. Managing " << entities.size() << " entities." << std::endl;
        if (!quadtree) {
            std::cout << "[CollisionSystem] Quadtree is null!" << std::endl;
            return; 
        }

        quadtree->clear();
        entitiesInsertedIntoQuadtree = 0; // Reset counter

        std::cout << "[CollisionSystem] Populating Quadtree..." << std::endl;
        for (auto const& entity : entities) {
            if (componentManager->hasComponent<TransformComponent>(entity) && 
                componentManager->hasComponent<RigidbodyComponent>(entity)) {
                auto& transform = componentManager->getComponent<TransformComponent>(entity);
                quadtree->insert(entity, transform);
                entitiesInsertedIntoQuadtree++;
            } else {
                std::cout << "[CollisionSystem] Entity " << entity << " skipped for Quadtree insertion (missing Transform or Rigidbody)." << std::endl;
            }
        }
        std::cout << "[CollisionSystem] Quadtree populated with " << entitiesInsertedIntoQuadtree << " entities." << std::endl;

        for (auto const& entityA : entities) {
            if (!componentManager->hasComponent<TransformComponent>(entityA) || 
                !componentManager->hasComponent<RigidbodyComponent>(entityA)) {
                // This check might be redundant if entities list is already filtered by SystemManager
                // but good for safety if entities can be added/removed dynamically without signature check.
                continue; 
            }

            auto& transformA = componentManager->getComponent<TransformComponent>(entityA);
            auto& rigidbodyA = componentManager->getComponent<RigidbodyComponent>(entityA);

            if (rigidbodyA.isStatic) continue; 

            SDL_Rect rectA = {(int)transformA.x, (int)transformA.y, (int)transformA.width, (int)transformA.height};
            std::cout << "[CollisionSystem] Processing Entity A: " << entityA << " rectA: (" << rectA.x << "," << rectA.y << "," << rectA.w << "," << rectA.h << ")" << std::endl;
            
            std::vector<Entity> potentialColliders = quadtree->query(transformA);
            std::cout << "[CollisionSystem] Entity A: " << entityA << " found " << potentialColliders.size() << " potential colliders from Quadtree." << std::endl;

            for (auto const& entityB : potentialColliders) {
                if (entityA == entityB) continue;

                if (!componentManager->hasComponent<TransformComponent>(entityB) || 
                    !componentManager->hasComponent<RigidbodyComponent>(entityB)) {
                    std::cout << "[CollisionSystem] Potential collider Entity B: " << entityB << " skipped (missing Transform or Rigidbody)." << std::endl;
                    continue;
                }

                auto& transformB = componentManager->getComponent<TransformComponent>(entityB);
                // auto& rigidbodyB = componentManager->getComponent<RigidbodyComponent>(entityB); // Already got this for quadtree insertion, but good to have if needed
                SDL_Rect rectB = {(int)transformB.x, (int)transformB.y, (int)transformB.width, (int)transformB.height};
                std::cout << "[CollisionSystem]   Checking against Entity B: " << entityB << " rectB: (" << rectB.x << "," << rectB.y << "," << rectB.w << "," << rectB.h << ")" << std::endl;

                if (Physics::checkCollision(rectA, rectB)) {
                    std::cout << "    Collision DETECTED between Entity A: " << entityA << " and Entity B: " << entityB << std::endl;

                    if (!componentManager->hasComponent<VelocityComponent>(entityA)) {
                        std::cout << "    Entity A: " << entityA << " missing VelocityComponent for collision response." << std::endl;
                        continue;
                    }
                    auto& velocityA = componentManager->getComponent<VelocityComponent>(entityA);

                    float oldX_A = transformA.x;
                    float oldY_A = transformA.y;

                    // Y-axis resolution
                    float overlapY = (transformA.y + transformA.height) - transformB.y;
                    float overlapY_BBelowA = (transformB.y + transformB.height) - transformA.y; 

                    if (rectA.y < rectB.y + rectB.h && rectA.y + rectA.h > rectB.y) { 
                        if (velocityA.vy > 0 && overlapY > 0 && overlapY < transformB.height) { 
                            std::cout << "      Response Y: Entity A ("" << entityA << "") moving DOWN into B ("" << entityB << ""). OverlapY: " << overlapY << std::endl;
                            transformA.y -= overlapY;
                            velocityA.vy = 0;
                        } else if (velocityA.vy < 0 && overlapY_BBelowA > 0 && overlapY_BBelowA < transformB.height) { 
                            std::cout << "      Response Y: Entity A ("" << entityA << "") moving UP into B ("" << entityB << ""). OverlapY_BBelowA: " << overlapY_BBelowA << std::endl;
                            transformA.y += overlapY_BBelowA;
                            velocityA.vy = 0;
                        }
                    }
                    
                    rectA.y = (int)transformA.y;

                    // X-axis resolution
                    float overlapXRight = (transformA.x + transformA.width) - transformB.x; 
                    float overlapXLeft = (transformB.x + transformB.width) - transformA.x; 

                    if (rectA.x < rectB.x + rectB.w && rectA.x + rectA.w > rectB.x) { 
                        if (velocityA.vx > 0 && overlapXRight > 0 && overlapXRight < transformB.width) {
                            std::cout << "      Response X: Entity A ("" << entityA << "") moving RIGHT into B ("" << entityB << ""). OverlapXRight: " << overlapXRight << std::endl;
                            transformA.x -= overlapXRight;
                            velocityA.vx = 0;
                        } else if (velocityA.vx < 0 && overlapXLeft > 0 && overlapXLeft < transformB.width) { 
                            std::cout << "      Response X: Entity A ("" << entityA << "") moving LEFT into B ("" << entityB << ""). OverlapXLeft: " << overlapXLeft << std::endl;
                            transformA.x += overlapXLeft;
                            velocityA.vx = 0;
                        }
                    }

                    if (oldX_A != transformA.x || oldY_A != transformA.y) {
                        std::cout << "      Position CHANGED for Entity A ("" << entityA << "") from ("" << oldX_A << "","" << oldY_A << "") to ("" << transformA.x << "","" << transformA.y << "")" << std::endl;
                    } else {
                        std::cout << "      Position NOT CHANGED for Entity A ("" << entityA << "") despite detected collision." << std::endl;
                    }
                }
            }
        }
    }
};
