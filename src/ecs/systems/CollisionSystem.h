#pragma once
#include "../System.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/RigidbodyComponent.h"
#include "../components/ColliderComponent.h"
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
        entitiesInsertedIntoQuadtree = 0;

        std::cout << "[CollisionSystem] Populating Quadtree..." << std::endl;

        for (auto const& entity : entities) {
            if (componentManager->hasComponent<TransformComponent>(entity) && 
                componentManager->hasComponent<ColliderComponent>(entity)) {
                auto& transform = componentManager->getComponent<TransformComponent>(entity);
                quadtree->insert(entity, transform);
                entitiesInsertedIntoQuadtree++;
            } else {
                std::cout << "[CollisionSystem] Entity " << entity << " skipped for Quadtree insertion (missing Transform or ColliderComponent)." << std::endl; // Updated log message
            }
        }
        std::cout << "[CollisionSystem] Quadtree populated with " << entitiesInsertedIntoQuadtree << " entities." << std::endl;

        for (auto const& entityA : entities) {
            if (!componentManager->hasComponent<TransformComponent>(entityA) ||
                !componentManager->hasComponent<ColliderComponent>(entityA)) {
                continue; 
            }

            auto& transformA = componentManager->getComponent<TransformComponent>(entityA);
            auto& colliderA = componentManager->getComponent<ColliderComponent>(entityA); 

            if (componentManager->hasComponent<RigidbodyComponent>(entityA)) {
                auto& rigidbodyA = componentManager->getComponent<RigidbodyComponent>(entityA);
                if (rigidbodyA.isStatic) {
                    std::cout << "[CollisionSystem] Entity A: " << entityA << " is static, skipping collision response." << std::endl;
                    continue;
                }
            }

            SDL_Rect rectA = {
                static_cast<int>(transformA.x + colliderA.offsetX),
                static_cast<int>(transformA.y + colliderA.offsetY),
                static_cast<int>(colliderA.width),
                static_cast<int>(colliderA.height)
            };
            std::cout << "[CollisionSystem] Processing Entity A: " << entityA << " rectA: (" << rectA.x << "," << rectA.y << "," << rectA.w << "," << rectA.h << ") using collider offset/dims" << std::endl;
            
            std::vector<Entity> potentialColliders = quadtree->query(transformA); 
            std::cout << "[CollisionSystem] Entity A: " << entityA << " found " << potentialColliders.size() << " potential colliders from Quadtree." << std::endl;

            for (auto const& entityB : potentialColliders) {
                if (entityA == entityB) continue;

                if (!componentManager->hasComponent<TransformComponent>(entityB) || 
                    !componentManager->hasComponent<ColliderComponent>(entityB)) {
                    std::cout << "[CollisionSystem] Potential collider Entity B: " << entityB << " skipped (missing Transform or ColliderComponent)." << std::endl; // Updated log
                    continue;
                }

                auto& transformB = componentManager->getComponent<TransformComponent>(entityB);
                auto& colliderB = componentManager->getComponent<ColliderComponent>(entityB); 
                
                SDL_Rect rectB = {
                    static_cast<int>(transformB.x + colliderB.offsetX),
                    static_cast<int>(transformB.y + colliderB.offsetY),
                    static_cast<int>(colliderB.width),
                    static_cast<int>(colliderB.height)
                };
                std::cout << "[CollisionSystem]   Checking against Entity B: " << entityB << " rectB: (" << rectB.x << "," << rectB.y << "," << rectB.w << "," << rectB.h << ") using collider offset/dims" << std::endl;

                if (Physics::checkCollision(rectA, rectB)) {
                    std::cout << "    Collision DETECTED between Entity A: " << entityA << " and Entity B: " << entityB << std::endl;

                    if (colliderA.isTrigger || colliderB.isTrigger) {
                        std::cout << "    Collision involves a trigger (A:" << colliderA.isTrigger << ", B:" << colliderB.isTrigger << "). Skipping physical response." << std::endl;
                        continue;
                    }

                    if (!componentManager->hasComponent<VelocityComponent>(entityA)) {
                        std::cout << "    Entity A: " << entityA << " missing VelocityComponent for collision response." << std::endl;
                        continue;
                    }
                    auto& velocityA = componentManager->getComponent<VelocityComponent>(entityA);

                    float oldTransformX_A = transformA.x; 
                    float oldTransformY_A = transformA.y;

                    if (velocityA.vy > 0) {
                        float penetration_y = (rectA.y + rectA.h) - rectB.y;
                        if (penetration_y > 0) {
                            // std::cout << "      Response Y (down): Entity A (" << entityA << ") into B (" << entityB << "). Penetration: " << penetration_y << std::endl;
                            transformA.y -= penetration_y; 
                            velocityA.vy = 0;
                            rectA.y = static_cast<int>(transformA.y + colliderA.offsetY); // Update rectA for X-axis check
                        }
                    } else if (velocityA.vy < 0) { 
                        float penetration_y = (rectB.y + rectB.h) - rectA.y;
                        if (penetration_y > 0) {
                            // std::cout << "      Response Y (up): Entity A (" << entityA << ") into B (" << entityB << "). Penetration: " << penetration_y << std::endl;
                            transformA.y += penetration_y;
                            velocityA.vy = 0;
                            rectA.y = static_cast<int>(transformA.y + colliderA.offsetY); 
                        }
                    }
                    
                    if (velocityA.vx > 0) { 
                        float penetration_x = (rectA.x + rectA.w) - rectB.x;
                        if (penetration_x > 0) {
                            // std::cout << "      Response X (right): Entity A (" << entityA << ") into B (" << entityB << "). Penetration: " << penetration_x << std::endl;
                            transformA.x -= penetration_x;
                            velocityA.vx = 0;
                            // rectA.x = static_cast<int>(transformA.x + colliderA.offsetX); // Update if more resolution steps followed
                        }
                    } else if (velocityA.vx < 0) { 
                        float penetration_x = (rectB.x + rectB.w) - rectA.x;
                        if (penetration_x > 0) {
                            // std::cout << "      Response X (left): Entity A (" << entityA << ") into B (" << entityB << "). Penetration: " << penetration_x << std::endl;
                            transformA.x += penetration_x; 
                            velocityA.vx = 0;
                            // rectA.x = static_cast<int>(transformA.x + colliderA.offsetX); // Update if more resolution steps followed
                        }
                    }

                    if (oldTransformX_A != transformA.x || oldTransformY_A != transformA.y) {
                        std::cout << "      Position CHANGED for Entity A ("" << entityA << "") from ("" << oldTransformX_A << "","" << oldTransformY_A << "") to ("" << transformA.x << "","" << transformA.y << "")" << std::endl;
                    } else {
                        std::cout << "      Position NOT CHANGED for Entity A ("" << entityA << "") despite detected collision." << std::endl;
                    }
                }
            }
        }
    }
};
