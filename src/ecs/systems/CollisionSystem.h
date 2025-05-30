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

// Define a struct for float-based rectangles
struct FloatRect {
    float x, y, w, h;
};

// Helper function for float-based AABB collision check
inline bool checkFloatAABBCollision(const FloatRect& r1, const FloatRect& r2) {
    return (r1.x < r2.x + r2.w &&
            r1.x + r1.w > r2.x &&
            r1.y < r2.y + r2.h &&
            r1.y + r1.h > r2.y);
}

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

        std::cout << "[CollisionSystem] Populating Quadtree..." << std::endl; // Fixed: removed backslash before quote

        for (auto const& entity : entities) {
            if (componentManager->hasComponent<TransformComponent>(entity) && 
                componentManager->hasComponent<ColliderComponent>(entity)) {
                auto& transform = componentManager->getComponent<TransformComponent>(entity);
                quadtree->insert(entity, transform); 
                entitiesInsertedIntoQuadtree++;
            } else {
                std::cout << "[CollisionSystem] Entity " << entity << " skipped for Quadtree insertion (missing Transform or ColliderComponent)." << std::endl; // Fixed: removed backslash before quote
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

            RigidbodyComponent* rigidbodyA_ptr = nullptr;
            if (componentManager->hasComponent<RigidbodyComponent>(entityA)) {
                rigidbodyA_ptr = &componentManager->getComponent<RigidbodyComponent>(entityA);
                if (!rigidbodyA_ptr->isStatic) {
                    rigidbodyA_ptr->isGrounded = false; // Assume not grounded until a collision proves it
                } else {
                    // Static bodies don't respond to collisions by moving
                    std::cout << "[CollisionSystem] Entity A: " << entityA << " is static, skipping collision response." << std::endl;
                    continue; 
                }
            }

            FloatRect float_rectA = {
                transformA.x + colliderA.offsetX,
                transformA.y + colliderA.offsetY,
                colliderA.width,
                colliderA.height
            };
            std::cout << "[CollisionSystem] Processing Entity A: " << entityA << " rectA: (" << float_rectA.x << "," << float_rectA.y << "," << float_rectA.w << "," << float_rectA.h << ")" << std::endl;
            
            std::vector<Entity> potentialColliders = quadtree->query(transformA); 
            std::cout << "[CollisionSystem] Entity A: " << entityA << " found " << potentialColliders.size() << " potential colliders from Quadtree." << std::endl;

            for (auto const& entityB : potentialColliders) {
                if (entityA == entityB) continue;

                if (!componentManager->hasComponent<TransformComponent>(entityB) || 
                    !componentManager->hasComponent<ColliderComponent>(entityB)) {
                    std::cout << "[CollisionSystem] Potential collider Entity B: " << entityB << " skipped (missing Transform or ColliderComponent)." << std::endl; // Fixed: removed backslash before quote
                    continue;
                }

                auto& transformB = componentManager->getComponent<TransformComponent>(entityB);
                auto& colliderB = componentManager->getComponent<ColliderComponent>(entityB); 
                
                FloatRect float_rectB = {
                    transformB.x + colliderB.offsetX,
                    transformB.y + colliderB.offsetY,
                    colliderB.width,
                    colliderB.height
                };
                std::cout << "[CollisionSystem]   Checking against Entity B: " << entityB << " rectB: (" << float_rectB.x << "," << float_rectB.y << "," << float_rectB.w << "," << float_rectB.h << ")" << std::endl;

                if (checkFloatAABBCollision(float_rectA, float_rectB)) {
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

                    if (velocityA.vy >= 0) { 
                        float penetration_y = (float_rectA.y + float_rectA.h) - float_rectB.y;
                        if (penetration_y > 0) {
                            bool primarily_vertical_collision = (float_rectA.x < float_rectB.x + float_rectB.w) && (float_rectA.x + float_rectA.w > float_rectB.x);
                            
                            if (primarily_vertical_collision) {
                                std::cout << "      Response Y (down/on): Entity A (" << entityA << ") into B (" << entityB << "). Penetration: " << penetration_y << std::endl;
                                transformA.y = transformB.y + colliderB.offsetY - colliderA.offsetY - colliderA.height;
                                velocityA.vy = 0;
                                if (rigidbodyA_ptr) {
                                    rigidbodyA_ptr->isGrounded = true;
                                }
                                float_rectA.y = transformA.y + colliderA.offsetY; 
                            }
                        }
                    }
                    if (velocityA.vy <= 0) { 
                        if (!(rigidbodyA_ptr && rigidbodyA_ptr->isGrounded && velocityA.vy == 0)) {
                            float penetration_y = (float_rectB.y + float_rectB.h) - float_rectA.y;
                            if (penetration_y > 0) {
                                bool primarily_vertical_collision = (float_rectA.x < float_rectB.x + float_rectB.w) && (float_rectA.x + float_rectA.w > float_rectB.x);
                                if (primarily_vertical_collision) {
                                    std::cout << "      Response Y (up): Entity A (" << entityA << ") into B (" << entityB << "). Penetration: " << penetration_y << std::endl;
                                    transformA.y = transformB.y + colliderB.offsetY + colliderB.height - colliderA.offsetY;
                                    velocityA.vy = 0;
                                    float_rectA.y = transformA.y + colliderA.offsetY; 
                                }
                            }
                        }
                    }

                    FloatRect current_float_rectA_for_X = {
                        transformA.x + colliderA.offsetX,
                        float_rectA.y, 
                        colliderA.width,
                        colliderA.height
                    };


                    if (velocityA.vx >= 0) { 
                        float penetration_x = (current_float_rectA_for_X.x + current_float_rectA_for_X.w) - float_rectB.x;
                        if (penetration_x > 0) {
                            bool primarily_horizontal_collision = (current_float_rectA_for_X.y < float_rectB.y + float_rectB.h) && (current_float_rectA_for_X.y + current_float_rectA_for_X.h > float_rectB.y);
                            if(primarily_horizontal_collision){
                                std::cout << "      Response X (right): Entity A (" << entityA << ") into B (" << entityB << "). Penetration: " << penetration_x << std::endl;
                                transformA.x = transformB.x + colliderB.offsetX - colliderA.offsetX - colliderA.width;
                                velocityA.vx = 0;
                            }
                        }
                    }
                     if (velocityA.vx <= 0) {
                        if (!(velocityA.vx == 0 && ((current_float_rectA_for_X.x + current_float_rectA_for_X.w) - float_rectB.x) > 0 )) {
                            float penetration_x = (float_rectB.x + float_rectB.w) - current_float_rectA_for_X.x;
                            if (penetration_x > 0) {
                                bool primarily_horizontal_collision = (current_float_rectA_for_X.y < float_rectB.y + float_rectB.h) && (current_float_rectA_for_X.y + current_float_rectA_for_X.h > float_rectB.y);
                                if(primarily_horizontal_collision){
                                    std::cout << "      Response X (left): Entity A (" << entityA << ") into B (" << entityB << "). Penetration: " << penetration_x << std::endl;
                                    transformA.x = transformB.x + colliderB.offsetX + colliderB.width - colliderA.offsetX; 
                                    velocityA.vx = 0;
                                }
                            }
                        }
                    }

                    if (oldTransformX_A != transformA.x || oldTransformY_A != transformA.y) {
                        std::cout << "      Position CHANGED for Entity A (" << entityA << ") from (" << oldTransformX_A << "," << oldTransformY_A << ") to (" << transformA.x << "," << transformA.y << ")" << std::endl;
                    } else {
                        std::cout << "      Position NOT CHANGED for Entity A (" << entityA << ") despite detected collision (may be fine if velocity was already zeroed or minor overlap)." << std::endl;
                    }
                }
            }
        }
    }
};
