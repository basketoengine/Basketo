#pragma once

#include "Types.h"
#include <set>

class System {
public:
    std::set<Entity> entities;

    virtual ~System() = default;

    virtual void addEntity(Entity entity) {
        entities.insert(entity);
    }

    virtual void removeEntity(Entity entity) {
        entities.erase(entity);
    }
    // Optionally, a virtual update method if systems need specific logic beyond iterating entities
    // virtual void update(ComponentManager* componentManager, float deltaTime) = 0; 
};
