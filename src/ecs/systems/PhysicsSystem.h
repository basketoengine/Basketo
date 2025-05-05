#pragma once
#include "../System.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/RigidbodyComponent.h"

class PhysicsSystem : public System {
public:
    float gravity = 980.0f; // pixels per second squared
    void update(ComponentManager* componentManager, float deltaTime) {
        for (auto const& entity : entities) {
            auto& velocity = componentManager->getComponent<VelocityComponent>(entity);
            auto& rigidbody = componentManager->getComponent<RigidbodyComponent>(entity);
            if (!rigidbody.isStatic && rigidbody.affectedByGravity) {
                velocity.vy += gravity * rigidbody.gravityScale * deltaTime;
            }
        }
    }
};
