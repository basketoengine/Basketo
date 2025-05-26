#include "PhysicsSystem.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/RigidbodyComponent.h"
#include <iostream> 

const float PhysicsSystem::GRAVITY_ACCELERATION = 980.0f;

void PhysicsSystem::init() {
}

void PhysicsSystem::update(ComponentManager* componentManager, float deltaTime) {
    for (auto const& entity : entities) { 
        if (!componentManager->hasComponent<VelocityComponent>(entity) || 
            !componentManager->hasComponent<RigidbodyComponent>(entity) ||
            !componentManager->hasComponent<TransformComponent>(entity)) { 
            continue;
        }

        auto& velocity = componentManager->getComponent<VelocityComponent>(entity);
        auto& rigidbody = componentManager->getComponent<RigidbodyComponent>(entity);
        auto& transform = componentManager->getComponent<TransformComponent>(entity);

        // Apply gravity
        if (rigidbody.useGravity) {
            velocity.vy += GRAVITY_ACCELERATION * rigidbody.gravityScale * deltaTime;
        }

        // Update position based on velocity
        transform.x += velocity.vx * deltaTime;
        transform.y += velocity.vy * deltaTime;
    }
}