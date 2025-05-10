#pragma once
#include "../System.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/RigidbodyComponent.h"

class PhysicsSystem : public System {
public:
    float gravity = 980.0f;
    void update(ComponentManager* componentManager, float deltaTime) {
        for (auto const& entity : entities) {
            auto& velocity = componentManager->getComponent<VelocityComponent>(entity);
            auto& rigidbody = componentManager->getComponent<RigidbodyComponent>(entity);
            if (!rigidbody.isStatic && rigidbody.useGravity) {
                velocity.vy += gravity * rigidbody.gravityScale * deltaTime;
            }
        }
    }
};
