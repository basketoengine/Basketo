#pragma once
#include <string>
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
