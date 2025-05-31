#pragma once

#include "../System.h"
#include "../Types.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../ComponentManager.h"

#include <iostream>

class MovementSystem : public System {
public:
    void update(ComponentManager* componentManager, float deltaTime) {
        for (auto const& entity : entities) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            auto& velocity = componentManager->getComponent<VelocityComponent>(entity);

            transform.x += velocity.vx * deltaTime;
        }
    }
};