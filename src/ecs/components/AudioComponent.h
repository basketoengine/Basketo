#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../../../vendor/nlohmann/json.hpp"

struct AudioComponent {
    std::string audioId;
    bool isMusic = false;
    bool playOnStart = false;
    bool loop = false;
    int volume = 128;
    bool isPlaying = false;

    AudioComponent() = default;
    AudioComponent(const std::string& id, bool music = false, bool play = false, bool l = false, int vol = 128)
        : audioId(id), isMusic(music), playOnStart(play), loop(l), volume(vol) {}
};

// New component for managing multiple sound effects per entity
struct SoundEffectsComponent {
    std::unordered_map<std::string, std::string> soundEffects; // action name -> audio ID
    int defaultVolume = 128;
    std::vector<std::string> playQueue; // Queue of sounds to play this frame

    SoundEffectsComponent() = default;

    // Add a sound effect for a specific action
    void addSoundEffect(const std::string& actionName, const std::string& audioId) {
        soundEffects[actionName] = audioId;
    }

    // Remove a sound effect
    void removeSoundEffect(const std::string& actionName) {
        soundEffects.erase(actionName);
    }

    // Queue a sound to be played this frame
    void playSound(const std::string& actionName) {
        if (soundEffects.find(actionName) != soundEffects.end()) {
            playQueue.push_back(actionName);
        }
    }

    // Get the audio ID for an action
    std::string getAudioId(const std::string& actionName) const {
        auto it = soundEffects.find(actionName);
        return (it != soundEffects.end()) ? it->second : "";
    }

    // Clear the play queue (called by AudioSystem after processing)
    void clearPlayQueue() {
        playQueue.clear();
    }
};

inline void to_json(nlohmann::json& j, const AudioComponent& c) {
    j = nlohmann::json{
        {"audioId", c.audioId},
        {"isMusic", c.isMusic},
        {"playOnStart", c.playOnStart},
        {"loop", c.loop},
        {"volume", c.volume}
    };
}

inline void from_json(const nlohmann::json& j, AudioComponent& c) {
    c.audioId = j.value("audioId", "");
    c.isMusic = j.value("isMusic", false);
    c.playOnStart = j.value("playOnStart", false);
    c.loop = j.value("loop", false);
    c.volume = j.value("volume", 128);
}

// JSON serialization for SoundEffectsComponent
inline void to_json(nlohmann::json& j, const SoundEffectsComponent& c) {
    j = nlohmann::json{
        {"soundEffects", c.soundEffects},
        {"defaultVolume", c.defaultVolume}
        // Note: playQueue is not serialized as it's runtime-only
    };
}

inline void from_json(const nlohmann::json& j, SoundEffectsComponent& c) {
    if (j.contains("soundEffects")) {
        c.soundEffects = j["soundEffects"].get<std::unordered_map<std::string, std::string>>();
    }
    c.defaultVolume = j.value("defaultVolume", 128);
    c.playQueue.clear(); // Always start with empty queue
}
