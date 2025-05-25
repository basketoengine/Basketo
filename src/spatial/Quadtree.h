\
#pragma once

#include <vector>
#include <memory>
#include <SDL2/SDL.h>
#include <iostream> // For logging I will remove this later
#include "../ecs/Entity.h"
#include "../ecs/components/TransformComponent.h"

using Entity = unsigned int; 

class Quadtree {
public:
    Quadtree(int level, SDL_Rect bounds);
    ~Quadtree(); // Add destructor declaration for logging
    void clear();
    void insert(Entity entity, const TransformComponent& transform); 
    std::vector<Entity> query(const TransformComponent& transform); 

private:
    static const int MAX_OBJECTS = 10;
    static const int MAX_LEVELS = 5;

    int currentLevel;
    std::vector<std::pair<Entity, TransformComponent>> objects; 
    SDL_Rect nodeBounds;
    std::unique_ptr<Quadtree> children[4]; 

    bool isLeaf() const;
    void split();
    int getIndex(const TransformComponent& transform) const;
    void retrieve(std::vector<Entity>& returnObjects, const TransformComponent& transform) const;
};
