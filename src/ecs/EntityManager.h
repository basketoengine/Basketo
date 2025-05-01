#pragma once
#include "Entity.h"
#include <queue>
#include <array>
#include "Types.h"

class EntityManager {
public:
    EntityManager() {
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
            availableEntities.push(entity);
    }

    Entity createEntity() {
        Entity id = availableEntities.front();
        availableEntities.pop();
        ++livingEntityCount;
        return id;
    }

    void destroyEntity(Entity entity) {
        signatures[entity].reset();
        availableEntities.push(entity);
        --livingEntityCount;
    }

    void setSignature(Entity entity, Signature signature) {
        signatures[entity] = signature;
    }

    Signature getSignature(Entity entity) const {
        return signatures[entity];
    }

private:
    std::queue<Entity> availableEntities;
    std::array<Signature, MAX_ENTITIES> signatures;
    uint32_t livingEntityCount = 0;
};
