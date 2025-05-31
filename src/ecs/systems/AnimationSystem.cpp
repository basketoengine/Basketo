#include "AnimationSystem.h"
#include "../EntityManager.h"
#include "../ComponentManager.h" 
#include "../components/AnimationComponent.h" 
#include "../components/SpriteComponent.h"

void AnimationSystem::update(float deltaTime, EntityManager& entityManager, ComponentManager& componentManager) {
    for (Entity entity : entityManager.getActiveEntities()) { 
        if (componentManager.hasComponent<AnimationComponent>(entity) && componentManager.hasComponent<SpriteComponent>(entity)) {
            auto& animComp = componentManager.getComponent<AnimationComponent>(entity);
            auto& spriteComp = componentManager.getComponent<SpriteComponent>(entity);

            if (!animComp.isPlaying || animComp.currentAnimationName.empty() || !animComp.animations.count(animComp.currentAnimationName)) {
                continue;
            }

            AnimationSequence& currentSeq = animComp.animations[animComp.currentAnimationName];
            if (currentSeq.frames.empty()) {
                continue;
            }

            if (animComp.currentFrameIndex == 0 && animComp.currentFrameTime < (deltaTime + 0.001f)) { // Adding a small epsilon for float comparison
                 std::cout << "[AnimationSystem] Entity " << entity 
                           << ": Processing animation '" << animComp.currentAnimationName 
                           << "' (Frame 0). TextureID: '" << currentSeq.textureId << "'" << std::endl;
            }

            animComp.currentFrameTime += deltaTime;
            while (animComp.isPlaying && animComp.currentFrameTime >= currentSeq.frames[animComp.currentFrameIndex].duration) {
                animComp.currentFrameTime -= currentSeq.frames[animComp.currentFrameIndex].duration;
                animComp.currentFrameIndex++;

                if (animComp.currentFrameIndex >= currentSeq.frames.size()) {
                    if (currentSeq.loop) {
                        animComp.currentFrameIndex = 0;
                    } else {
                        animComp.isPlaying = false;
                        animComp.currentFrameIndex = currentSeq.frames.size() - 1;
                        break;
                    }
                }
            }

            if (animComp.isPlaying) {
                spriteComp.srcRect = currentSeq.frames[animComp.currentFrameIndex].sourceRect;
                spriteComp.textureId = currentSeq.textureId; 
                spriteComp.useSrcRect = true; 
                
                spriteComp.flip = SDL_FLIP_NONE;
                if (animComp.flipHorizontal) {
                    spriteComp.flip = (SDL_RendererFlip)(spriteComp.flip | SDL_FLIP_HORIZONTAL);
                }
                if (animComp.flipVertical) {
                    spriteComp.flip = (SDL_RendererFlip)(spriteComp.flip | SDL_FLIP_VERTICAL);
                }

                std::cout << "[AnimationSystem] Entity " << entity << " flipHorizontal: " << animComp.flipHorizontal << " spriteComp.flip: " << spriteComp.flip << std::endl;
            }
        }
    }
}