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

        // Static bodies are not processed by the physics simulation for movement
        if (rigidbody.isStatic) {
            continue;
        }

        auto& velocity = componentManager->getComponent<VelocityComponent>(entity);
        auto& transform = componentManager->getComponent<TransformComponent>(entity);

        // Kinematic bodies are controlled by scripts or animation.
        // Physics system should not apply gravity or integrate their position if they are kinematic.
        // MovementSystem will handle their position updates based on velocity set by scripts.
        if (rigidbody.isKinematic) {
            // For kinematic bodies, we skip gravity application and position integration here.
            // Their velocity is set externally (e.g., by Lua script),
            // and MovementSystem will use that velocity to update the transform.
            // If you still want kinematic bodies to be affected by *some* physics calculations
            // (e.g., responding to collisions by stopping, but not by forces),
            // that logic would be handled in CollisionSystem or by script.
        } else {
            // For dynamic (non-kinematic, non-static) bodies:
            // Apply gravity only if not grounded
            if (rigidbody.useGravity && !rigidbody.isGrounded) {
                velocity.vy += GRAVITY_ACCELERATION * rigidbody.gravityScale * deltaTime;
            }
            // CollisionSystem is responsible for setting isGrounded to true or false each frame.
        }
    }
}