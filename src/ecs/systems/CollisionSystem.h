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
                if (rigidbodyA_ptr->isStatic) { 
                    continue; 
                }
            }
            if (componentManager->hasComponent<ColliderComponent>(entityA)) {
                auto& colliderA_comp = componentManager->getComponent<ColliderComponent>(entityA);
                colliderA_comp.contacts.clear();
            }

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
                    Entity entityB_swept_collider = NO_ENTITY;

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
                                entityB_swept_collider = entityB;
                                break; 
                            }
                        } else if (velocityA.vy < 0) { 
                            startSweptY = oldY + colliderA.offsetY;
                            endSweptY = newY + colliderA.offsetY;
                            if (verticalLineIntersectsAABB(sweptPointX, startSweptY, endSweptY, float_rectB, collisionResolutionY)) {

                                sweptY = collisionResolutionY - colliderA.offsetY;
                                sweptCollision = true;
                                entityB_swept_collider = entityB;
                                break;
                            }
                        }
                    }

                    if (sweptCollision) {
                        float originalVy = velocityA.vy;
                        transformA.y = sweptY;
                        velocityA.vy = 0; 

                        if (componentManager->hasComponent<ColliderComponent>(entityA) && entityB_swept_collider != NO_ENTITY) {
                            auto& colliderA_comp = componentManager->getComponent<ColliderComponent>(entityA);
                            Vec2D normal = {0, 0};
                            if (originalVy > 0) normal.y = -1;
                            else if (originalVy < 0) normal.y = 1;
                            colliderA_comp.contacts.push_back({entityB_swept_collider, normal});
                        }
                        if (componentManager->hasComponent<ColliderComponent>(entityB_swept_collider)) {
                             auto& colliderB_comp = componentManager->getComponent<ColliderComponent>(entityB_swept_collider);
                             Vec2D normal = {0, 0};
                             if (originalVy > 0) normal.y = 1;
                             else if (originalVy < 0) normal.y = -1;
                             colliderB_comp.contacts.push_back({entityA, normal});
                        }
                       
                        std::cout << "[CCD] Entity " << entityA << " collided with " << entityB_swept_collider << ". SweptY: " << sweptY << ", OriginalVy: " << originalVy << std::endl;

                    } else {
                        transformA.y = newY;
                    }
                } else { 
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
                             
                             auto& tB = componentManager->getComponent<TransformComponent>(entityB);
                             auto& cB = componentManager->getComponent<ColliderComponent>(entityB);
                             if (colliderA.isTrigger || cB.isTrigger) continue;

                             FloatRect float_rectB = {
                                 tB.x + cB.offsetX,
                                 tB.y + cB.offsetY,
                                 cB.width,
                                 cB.height
                             };
                             if(checkFloatAABBCollision(checkRectA, float_rectB)){
                                if (componentManager->hasComponent<ColliderComponent>(entityA)) {
                                    auto& colliderA_comp = componentManager->getComponent<ColliderComponent>(entityA);
                                    colliderA_comp.contacts.push_back({entityB, {0, -1}}); 
                                }
                                if (componentManager->hasComponent<ColliderComponent>(entityB)) {
                                    auto& colliderB_comp = componentManager->getComponent<ColliderComponent>(entityB);
                                    colliderB_comp.contacts.push_back({entityA, {0, 1}});
                                }
                                std::cout << "[Discrete Check] Entity " << entityA << " in resting contact with " << entityB << std::endl;
                               
                                break;
                             }
                        }
                }
            }
        }
    }
};
