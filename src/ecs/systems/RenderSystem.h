#pragma once

#include <SDL2/SDL.h>
#include "../System.h"
#include "../components/TransformComponent.h"
#include "../components/SpriteComponent.h" // Include SpriteComponent
#include "../ComponentManager.h"
#include "../../AssetManager.h" // Include AssetManager
#include <iostream>

class RenderSystem : public System {
public:
    void update(SDL_Renderer* renderer, ComponentManager* componentManager) {
        // Get the AssetManager instance once
        AssetManager& assetManager = AssetManager::getInstance();

        for (auto const& entity : entities) {
            // Get required components
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            auto& sprite = componentManager->getComponent<SpriteComponent>(entity);

            // Get the texture from the AssetManager
            SDL_Texture* texture = assetManager.getTexture(sprite.textureId);

            if (!texture) {
                std::cerr << "RenderSystem Error: Texture not found for ID: " << sprite.textureId << std::endl;
                continue; // Skip rendering this entity if texture is missing
            }

            // Destination rectangle based on TransformComponent
            SDL_Rect destRect;
            destRect.x = static_cast<int>(transform.x);
            destRect.y = static_cast<int>(transform.y);
            destRect.w = static_cast<int>(transform.width);
            destRect.h = static_cast<int>(transform.height);

            // Source rectangle (optional)
            SDL_Rect* srcRectPtr = sprite.useSrcRect ? &sprite.srcRect : nullptr;

            // Render the texture
            // Note: SDL_RenderCopyEx can be used later for rotation/flipping
            SDL_RenderCopy(renderer, texture, srcRectPtr, &destRect);
        }
    }
};
