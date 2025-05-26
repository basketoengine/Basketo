#pragma once
#include "../System.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/RigidbodyComponent.h"

class PhysicsSystem : public System {
public:
    static const float GRAVITY_ACCELERATION;
    void init();
    void update(ComponentManager* componentManager, float deltaTime);
};
