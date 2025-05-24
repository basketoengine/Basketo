\
#include "Quadtree.h"

Quadtree::Quadtree(int level, SDL_Rect bounds)
    : currentLevel(level), nodeBounds(bounds) {
}

Quadtree::~Quadtree() {
    clear(); 
}

void Quadtree::clear() {
    objects.clear();

    for (int i = 0; i < 4; ++i) {
        if (children[i] != nullptr) {
            children[i]->clear();
            children[i].reset(); 
        }
    }
}

void Quadtree::insert(Entity entity, const TransformComponent& transform) {
    if (!isLeaf()) {
        int index = getIndex(transform);
        if (index != -1) {
            children[index]->insert(entity, transform);
            return;
        }
    }

    objects.emplace_back(entity, transform);

    if (objects.size() > MAX_OBJECTS && currentLevel < MAX_LEVELS) {
        if (isLeaf()) { 
            split();
        }

        int i = 0;
        while (i < objects.size()) {
            int index = getIndex(objects[i].second);
            if (index != -1) {
                children[index]->insert(objects[i].first, objects[i].second);
                objects.erase(objects.begin() + i); 
            } else {
                i++; 
            }
        }
    }
}

std::vector<Entity> Quadtree::query(const TransformComponent& transform) {
    std::vector<Entity> returnObjects;
    retrieve(returnObjects, transform);
    return returnObjects; 
}

bool Quadtree::isLeaf() const {
    return children[0] == nullptr;
}

void Quadtree::split() {
    int subWidth = nodeBounds.w / 2;
    int subHeight = nodeBounds.h / 2;
    int x = nodeBounds.x;
    int y = nodeBounds.y;

    
    children[0] = std::make_unique<Quadtree>(currentLevel + 1, SDL_Rect{x, y, subWidth, subHeight});
    
    children[1] = std::make_unique<Quadtree>(currentLevel + 1, SDL_Rect{x + subWidth, y, subWidth, subHeight});
    
    children[2] = std::make_unique<Quadtree>(currentLevel + 1, SDL_Rect{x, y + subHeight, subWidth, subHeight});
    
    children[3] = std::make_unique<Quadtree>(currentLevel + 1, SDL_Rect{x + subWidth, y + subHeight, subWidth, subHeight});
}

int Quadtree::getIndex(const TransformComponent& transform) const {
    int index = -1;
    float objX = transform.x;
    float objY = transform.y;
    float objWidth = transform.width;
    float objHeight = transform.height;

    float verticalMidpoint = nodeBounds.x + (nodeBounds.w / 2.0f);
    float horizontalMidpoint = nodeBounds.y + (nodeBounds.h / 2.0f);

    bool topQuadrant = (objY < horizontalMidpoint && (objY + objHeight) < horizontalMidpoint);
    
    bool bottomQuadrant = (objY > horizontalMidpoint);

    if (objX < verticalMidpoint && (objX + objWidth) < verticalMidpoint) {
        if (topQuadrant) {
            index = 0; 
        } else if (bottomQuadrant) {
            index = 2; 
        }
    }

    else if (objX > verticalMidpoint) {
        if (topQuadrant) {
            index = 1; 
        } else if (bottomQuadrant) {
            index = 3; 
        }
    }
    return index;
}

void Quadtree::retrieve(std::vector<Entity>& returnObjects, const TransformComponent& transform) const {
    for (const auto& pair : objects) {
        returnObjects.push_back(pair.first);
    }

    if (!isLeaf()) {
        int index = getIndex(transform);

        if (index != -1) {
            children[index]->retrieve(returnObjects, transform);
        } else {
            SDL_Rect queryRect = {
                static_cast<int>(transform.x),
                static_cast<int>(transform.y),
                static_cast<int>(transform.width),
                static_cast<int>(transform.height)
            };

            for (int i = 0; i < 4; ++i) {
                if (SDL_HasIntersection(&children[i]->nodeBounds, &queryRect)) {
                    children[i]->retrieve(returnObjects, transform);
                }
            }
        }
    }
}
