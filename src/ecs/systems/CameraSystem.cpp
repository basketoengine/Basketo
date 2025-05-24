\
#include "CameraSystem.h"
#include <iostream> 
#include <SDL2/SDL_render.h> 

CameraSystem::CameraSystem(ComponentManager* componentManager, EntityManager* entityManager, SDL_Renderer* renderer)
    : componentManager_(componentManager), entityManager_(entityManager), renderer_(renderer) {
}

void CameraSystem::update(SDL_Rect& outWorldViewTopLeft_WorldSpace, float& outZoom) {
    activeCameraEntity_ = NO_ENTITY;
    for (Entity entity : entityManager_->getActiveEntities()) {
        if (componentManager_->hasComponent<CameraComponent>(entity)) {
            auto& camComp = componentManager_->getComponent<CameraComponent>(entity);
            if (camComp.isActive) {
                activeCameraEntity_ = entity;
                break; 
            }
        }
    }

    if (activeCameraEntity_ != NO_ENTITY) {
        auto& camComp = componentManager_->getComponent<CameraComponent>(activeCameraEntity_); 
        if (!componentManager_->hasComponent<TransformComponent>(activeCameraEntity_)) {
            std::cerr << "CameraSystem Error: Active CameraComponent (Entity " << activeCameraEntity_ << ") lacks a TransformComponent." << std::endl;
            
            int w, h;
            SDL_GetRendererOutputSize(renderer_, &w, &h);
            outWorldViewTopLeft_WorldSpace = {0, 0, w, h};
            outZoom = 1.0f;
            activeCameraEntity_ = NO_ENTITY;
            return;
        }
        auto& transform = componentManager_->getComponent<TransformComponent>(activeCameraEntity_);

        float worldVisibleWidth = camComp.width / camComp.zoom;
        float worldVisibleHeight = camComp.height / camComp.zoom;

        outWorldViewTopLeft_WorldSpace.x = static_cast<int>(transform.x - worldVisibleWidth / 2.0f);
        outWorldViewTopLeft_WorldSpace.y = static_cast<int>(transform.y - worldVisibleHeight / 2.0f);
        outWorldViewTopLeft_WorldSpace.w = static_cast<int>(worldVisibleWidth);
        outWorldViewTopLeft_WorldSpace.h = static_cast<int>(worldVisibleHeight);
        outZoom = camComp.zoom;

    } else {
        int w, h;
        SDL_GetRendererOutputSize(renderer_, &w, &h); 
        outWorldViewTopLeft_WorldSpace.x = 0;
        outWorldViewTopLeft_WorldSpace.y = 0; 
        outWorldViewTopLeft_WorldSpace.w = w;
        outWorldViewTopLeft_WorldSpace.h = h;
        outZoom = 1.0f;
    }
}

Entity CameraSystem::getActiveCameraEntity() const {
    return activeCameraEntity_;
}
