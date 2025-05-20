#pragma once
#include "../System.h"
#include "../EntityManager.h"
#include "../ComponentManager.h"

class AudioSystem : public System {
public:
    void update(float deltaTime, EntityManager& entityManager, ComponentManager& componentManager);
};
