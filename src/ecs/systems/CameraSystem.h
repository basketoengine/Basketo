\
#pragma once

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rect.h>
#include <vector>

#include "../Entity.h"
#include "../ComponentManager.h" 
#include "../EntityManager.h" 
#include "../System.h"
#include "../components/CameraComponent.h"
#include "../components/TransformComponent.h"

class CameraSystem : public System {
public:
    CameraSystem(ComponentManager* componentManager, EntityManager* entityManager, SDL_Renderer* renderer);
    void update(SDL_Rect& viewport, float& zoom);
    Entity getActiveCameraEntity() const;

private:
    ComponentManager* componentManager_;
    EntityManager* entityManager_;
    SDL_Renderer* renderer_;
    Entity activeCameraEntity_ = NO_ENTITY; 
};
