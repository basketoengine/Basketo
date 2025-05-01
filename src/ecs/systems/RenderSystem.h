#pragma once

#include <SDL2/SDL.h>
#include "../System.h"
#include "../components/TransformComponent.h"
#include "../ComponentManager.h"
#include <iostream>

class RenderSystem : public System {
public:
    void update(SDL_Renderer* renderer, ComponentManager* componentManager) {
        for (auto const& entity : entities) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);

            SDL_Rect rect;
            rect.x = static_cast<int>(transform.x);
            rect.y = static_cast<int>(transform.y);
            rect.w = static_cast<int>(transform.width);
            rect.h = static_cast<int>(transform.height);


            if (entity == 0) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
            }
            SDL_RenderFillRect(renderer, &rect);
        }
    }
};
