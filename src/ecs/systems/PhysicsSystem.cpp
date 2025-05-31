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

        auto& rigidbody = componentManager->getComponent<RigidbodyComponent>(entity);
        if (rigidbody.isStatic) {
            continue;
        }

        auto& velocity = componentManager->getComponent<VelocityComponent>(entity);
      
        if (rigidbody.isKinematic) {
            // Kinematic bodies are controlled by scripts or animation.
            // Their velocity is set externally. PhysicsSystem does not apply forces like gravity to them.
        } else { // Dynamic (non-kinematic, non-static) bodies:
            // Always apply gravity. Lua script will counteract it if grounded.
            if (rigidbody.useGravity) {
                velocity.vy += GRAVITY_ACCELERATION * rigidbody.gravityScale * deltaTime;
            }
        }
    }
}