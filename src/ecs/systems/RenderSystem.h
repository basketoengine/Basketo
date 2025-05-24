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
    AssetManager& assetManager;

    RenderSystem() : assetManager(AssetManager::getInstance()) {}

    void update(SDL_Renderer* renderer, ComponentManager* componentManager, float cameraX, float cameraY) {
        int screenWidth, screenHeight;
        if (SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight) != 0) {
            std::cerr << "RenderSystem Error: Could not get renderer output size. Culling might be ineffective." << std::endl;
            screenWidth = 1920;
            screenHeight = 1080;
        }

        for (auto const& entity : entities) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            auto& sprite = componentManager->getComponent<SpriteComponent>(entity);

            SDL_Rect destRect;
            destRect.x = static_cast<int>(transform.x - cameraX);
            destRect.y = static_cast<int>(transform.y - cameraY);
            destRect.w = static_cast<int>(transform.width);
            destRect.h = static_cast<int>(transform.height);

            if (destRect.x + destRect.w <= 0 ||
                destRect.x >= screenWidth   ||
                destRect.y + destRect.h <= 0 ||
                destRect.y >= screenHeight) {  
                continue;
            }

            SDL_Texture* texture = assetManager.getTexture(sprite.textureId);

            if (!texture) {
                std::cerr << "RenderSystem Error: Texture not found for ID: " << sprite.textureId << std::endl;
                continue;
            }

            SDL_Rect* srcRectPtr = sprite.useSrcRect ? &sprite.srcRect : nullptr;

            // Use SDL_RenderCopyEx for rotation
            SDL_Point center = { static_cast<int>(destRect.w / 2.0f), static_cast<int>(destRect.h / 2.0f) };
            SDL_RenderCopyEx(renderer, texture, srcRectPtr, &destRect, transform.rotation, &center, sprite.flip);
        }
    }
};
