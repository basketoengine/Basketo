#pragma once

#include <SDL2/SDL.h>
#include "../System.h"
#include "../components/TransformComponent.h"
#include "../components/SpriteComponent.h"
#include "../ComponentManager.h"
#include "../../AssetManager.h"
#include <iostream>

class RenderSystem : public System {
public:
    void update(SDL_Renderer* renderer, ComponentManager* componentManager) {
        AssetManager& assetManager = AssetManager::getInstance();

        for (auto const& entity : entities) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            auto& sprite = componentManager->getComponent<SpriteComponent>(entity);

            SDL_Texture* texture = assetManager.getTexture(sprite.textureId);

            if (!texture) {
                std::cerr << "RenderSystem Error: Texture not found for ID: " << sprite.textureId << std::endl;
                continue;
            }

            SDL_Rect destRect;
            destRect.x = static_cast<int>(transform.x);
            destRect.y = static_cast<int>(transform.y);
            destRect.w = static_cast<int>(transform.width);
            destRect.h = static_cast<int>(transform.height);

            SDL_Rect* srcRectPtr = sprite.useSrcRect ? &sprite.srcRect : nullptr;

            SDL_RenderCopy(renderer, texture, srcRectPtr, &destRect);
        }
    }
};
