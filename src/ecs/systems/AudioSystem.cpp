#include "AudioSystem.h"
#include "../components/AudioComponent.h"
#include "../../AssetManager.h"
#include <SDL2/SDL_mixer.h>

void AudioSystem::update(float deltaTime, EntityManager& entityManager, ComponentManager& componentManager) {
    for (Entity entity : entityManager.getActiveEntities()) {
        if (componentManager.hasComponent<AudioComponent>(entity)) {
            auto& audioComp = componentManager.getComponent<AudioComponent>(entity);
            if (audioComp.playOnStart && !audioComp.isPlaying && !audioComp.audioId.empty()) {
                if (audioComp.isMusic) {
                    Mix_Music* music = AssetManager::getInstance().getMusic(audioComp.audioId);
                    if (music) {
                        Mix_VolumeMusic(audioComp.volume);
                        Mix_PlayMusic(music, audioComp.loop ? -1 : 1);
                        audioComp.isPlaying = true;
                    }
                } else {
                    Mix_Chunk* chunk = AssetManager::getInstance().getSound(audioComp.audioId);
                    if (chunk) {
                        Mix_VolumeChunk(chunk, audioComp.volume);
                        Mix_PlayChannel(-1, chunk, audioComp.loop ? -1 : 0);
                        audioComp.isPlaying = true;
                    }
                }
            }
        }
    }
}
