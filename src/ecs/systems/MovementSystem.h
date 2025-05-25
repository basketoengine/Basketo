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
            // std::cout << "[MovementSystem] Entity " << entity << " BEFORE: pos(" << transform.x << "," << transform.y << ") vel(" << velocity.vx << "," << velocity.vy << ")" << std::endl;
        }

        for (auto const& entity : entities) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            auto& velocity = componentManager->getComponent<VelocityComponent>(entity);

            transform.x += velocity.vx * deltaTime;
            transform.y += velocity.vy * deltaTime;
            // std::cout << "[MovementSystem] Entity " << entity << " AFTER: pos(" << transform.x << "," << transform.y << ")" << std::endl;
        }
    }
};