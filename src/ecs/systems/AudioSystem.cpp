#include "AudioSystem.h"
#include "../components/AudioComponent.h"
#include "../../AssetManager.h"
#include <SDL2/SDL_mixer.h>
#include <iostream>

void AudioSystem::update(float deltaTime, EntityManager& entityManager, ComponentManager& componentManager) {
    for (Entity entity : entityManager.getActiveEntities()) {
        // Handle traditional AudioComponent (for background music, etc.)
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

        // Handle new SoundEffectsComponent (for multiple sound effects)
        if (componentManager.hasComponent<SoundEffectsComponent>(entity)) {
            auto& soundEffectsComp = componentManager.getComponent<SoundEffectsComponent>(entity);

            // Process all queued sounds
            for (const std::string& actionName : soundEffectsComp.playQueue) {
                std::string audioId = soundEffectsComp.getAudioId(actionName);
                if (!audioId.empty()) {
                    Mix_Chunk* chunk = AssetManager::getInstance().getSound(audioId);
                    if (chunk) {
                        Mix_VolumeChunk(chunk, soundEffectsComp.defaultVolume);
                        int channel = Mix_PlayChannel(-1, chunk, 0); // Sound effects don't loop by default
                        if (channel == -1) {
                            std::cerr << "AudioSystem: Failed to play sound '" << audioId << "' for action '" << actionName << "'. Mix_Error: " << Mix_GetError() << std::endl;
                        }
                    } else {
                        std::cerr << "AudioSystem: Sound '" << audioId << "' not found for action '" << actionName << "'" << std::endl;
                    }
                }
            }

            // Clear the play queue after processing
            soundEffectsComp.clearPlayQueue();
        }
    }
}
