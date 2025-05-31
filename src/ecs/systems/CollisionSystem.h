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

struct FloatRect {
    float x, y, w, h;
};

inline bool checkFloatAABBCollision(const FloatRect& r1, const FloatRect& r2) {
    return (r1.x < r2.x + r2.w &&
            r1.x + r1.w > r2.x &&
            r1.y < r2.y + r2.h &&
            r1.y + r1.h > r2.y);
}

inline bool verticalLineIntersectsAABB(float x, float y0, float y1, const FloatRect& rect, float& outY) {

    if (x < rect.x || x > rect.x + rect.w) return false;

    float minY = std::min(y0, y1);
    float maxY = std::max(y0, y1);
    if (maxY < rect.y || minY > rect.y + rect.h) return false;

    if (y1 > y0) {
        outY = rect.y - 0.001f;
    } else { 
        outY = rect.y + rect.h + 0.001f;
    }
    return true;
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

        std::cout << "[CollisionSystem] Populating Quadtree..." << std::endl; 

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
                    rigidbodyA_ptr->isGrounded = false; 
                } else {
                    continue; 
                }
            }

            // Clamp max fall velocity to prevent tunneling (general safety)
            if (componentManager->hasComponent<VelocityComponent>(entityA)) {
                auto& velocityA = componentManager->getComponent<VelocityComponent>(entityA);
                const float MAX_FALL_SPEED = 1200.0f; 
                if (velocityA.vy > MAX_FALL_SPEED) velocityA.vy = MAX_FALL_SPEED;
            }
            
            std::vector<Entity> potentialColliders = quadtree->query(transformA); 
            // std::cout << "[CollisionSystem] Entity A: " << entityA << " found " << potentialColliders.size() << " potential colliders from Quadtree." << std::endl; // Optional: keep for debugging

            if (componentManager->hasComponent<VelocityComponent>(entityA) && rigidbodyA_ptr && !rigidbodyA_ptr->isStatic) {
                auto& velocityA = componentManager->getComponent<VelocityComponent>(entityA);

                if (velocityA.vy != 0.0f) { 
                    float oldY = transformA.y;
                    float newY = transformA.y + velocityA.vy * deltaTime;
                    float sweptY = newY; 
                    bool sweptCollision = false;

                    for (auto const& entityB : potentialColliders) {
                        if (entityA == entityB) continue;
                        if (!componentManager->hasComponent<TransformComponent>(entityB) ||
                            !componentManager->hasComponent<ColliderComponent>(entityB)) continue;
                        
                        auto& colliderB_check = componentManager->getComponent<ColliderComponent>(entityB);
                        if (colliderA.isTrigger || colliderB_check.isTrigger) {
                            // std::cout << "[CCD] Entity " << entityA << " vs " << entityB << " involves a trigger. Skipping CCD solid response." << std::endl; // Optional debug
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
                        
                        float collisionResolutionY = 0.0f;
                        
                        float sweptPointX = transformA.x + colliderA.offsetX + colliderA.width / 2.0f;
                        float startSweptY, endSweptY;

                        if (velocityA.vy > 0) {
                            startSweptY = oldY + colliderA.offsetY + colliderA.height;
                            endSweptY = newY + colliderA.offsetY + colliderA.height; 
                            if (verticalLineIntersectsAABB(sweptPointX, startSweptY, endSweptY, float_rectB, collisionResolutionY)) {
                                sweptY = collisionResolutionY - (colliderA.offsetY + colliderA.height);
                                sweptCollision = true;
                                break; 
                            }
                        } else if (velocityA.vy < 0) { 
                            startSweptY = oldY + colliderA.offsetY;
                            endSweptY = newY + colliderA.offsetY;
                            if (verticalLineIntersectsAABB(sweptPointX, startSweptY, endSweptY, float_rectB, collisionResolutionY)) {

                                sweptY = collisionResolutionY - colliderA.offsetY;
                                sweptCollision = true;
                                break;
                            }
                        }
                    }

                    if (sweptCollision) {
                        float originalVy = velocityA.vy;
                        transformA.y = sweptY;
                        velocityA.vy = 0; 
                        if (rigidbodyA_ptr) {
                           if (originalVy > 0) {
                               rigidbodyA_ptr->isGrounded = true;
                               std::cout << "[CCD] Entity " << entityA << " SET TO GROUNDED (downward collision). OriginalVy: " << originalVy << ", SweptY: " << sweptY << std::endl;
                           } else {
                               std::cout << "[CCD] Entity " << entityA << " COLLIDED (not a grounding downward collision). OriginalVy: " << originalVy << ", SweptY: " << sweptY << std::endl;
                           }
                        }
                    } else {
                        transformA.y = newY;
                        std::cout << "[CCD] Entity " << entityA << " NO swept collision. Moved to newY: " << newY << ". isGrounded remains false (from loop start)." << std::endl;
                    }
                } else {
                    if (rigidbodyA_ptr) {
                        bool currentlyOverlappingGround = false;
                        FloatRect checkRectA = {
                            transformA.x + colliderA.offsetX,
                            transformA.y + colliderA.offsetY + 0.5f,
                            colliderA.width,
                            colliderA.height - 0.5f
                        };
                        if (colliderA.height <= 0.5f) {
                            checkRectA.h = 0.1f;
                        }

                        for (auto const& entityB : potentialColliders) {
                             if (entityA == entityB) continue;
                             if (!componentManager->hasComponent<TransformComponent>(entityB) ||
                                 !componentManager->hasComponent<ColliderComponent>(entityB)) continue;
                             auto& colliderB_check = componentManager->getComponent<ColliderComponent>(entityB);
                             if (colliderA.isTrigger || colliderB_check.isTrigger) continue;

                             auto& transformB = componentManager->getComponent<TransformComponent>(entityB);
                             FloatRect float_rectB = {
                                 transformB.x + colliderB_check.offsetX,
                                 transformB.y + colliderB_check.offsetY,
                                 colliderB_check.width,
                                 colliderB_check.height
                             };
                             if(checkFloatAABBCollision(checkRectA, float_rectB)){
                                currentlyOverlappingGround = true;
                                break;
                             }
                        }
                        
                        if (currentlyOverlappingGround) {
                            rigidbodyA_ptr->isGrounded = true;
                            std::cout << "[Discrete Ground Check] Entity " << entityA << " SET TO GROUNDED by overlap (vy=0)." << std::endl;
                        } else {
                            if (rigidbodyA_ptr->isGrounded) {
                                std::cout << "[Discrete Ground Check] Entity " << entityA << " was GROUNDED by CCD, but discrete check says NO overlap. isGrounded might be set false if not already." << std::endl;
                            }
                            if (!rigidbodyA_ptr->isGrounded) {
                                std::cout << "[Discrete Ground Check] Entity " << entityA << " NOT grounded by overlap (vy=0). isGrounded remains false." << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }
};
