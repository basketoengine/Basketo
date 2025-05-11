#include "PhysicsSystem.h"
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

        if (rigidbody.affectedByGravity) {
            velocity.vy += GRAVITY_ACCELERATION * rigidbody.gravityScale * deltaTime;
        }

    }
}