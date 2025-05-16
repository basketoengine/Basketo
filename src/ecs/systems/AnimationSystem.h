#pragma once
#include "../System.h"
#include "../EntityManager.h"
#include "../ComponentManager.h"
#include "../components/AnimationComponent.h"
#include "../components/SpriteComponent.h" 

class AnimationSystem : public System {
public:
    void update(float deltaTime, EntityManager& entityManager, ComponentManager& componentManager);
};