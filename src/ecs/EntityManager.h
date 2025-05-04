#pragma once
#include "Entity.h"
#include <queue>
#include <array>
#include <set>
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
        activeEntities.insert(id);
        return id;
    }

    void destroyEntity(Entity entity) {
        signatures[entity].reset();
        availableEntities.push(entity);
        --livingEntityCount;
        activeEntities.erase(entity);
    }

    void setSignature(Entity entity, Signature signature) {
        signatures[entity] = signature;
    }

    Signature getSignature(Entity entity) const {
        return signatures[entity];
    }

    const std::set<Entity>& getActiveEntities() const {
        return activeEntities;
    }

private:
    std::queue<Entity> availableEntities;
    std::array<Signature, MAX_ENTITIES> signatures;
    uint32_t livingEntityCount = 0;
    std::set<Entity> activeEntities;
};
