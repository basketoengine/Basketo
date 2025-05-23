#pragma once
#include <string>
#include <vector>
#include "SDL.h"
#include "../../vendor/nlohmann/json.hpp" 

namespace nlohmann {
    template <typename T>
    struct adl_serializer;
}

// SDL_Rect serialization
inline void to_json(nlohmann::json& j, const SDL_Rect& rect) {
    j = nlohmann::json{{"x", rect.x}, {"y", rect.y}, {"w", rect.w}, {"h", rect.h}};
}

inline void from_json(const nlohmann::json& j, SDL_Rect& rect) {
    j.at("x").get_to(rect.x);
    j.at("y").get_to(rect.y);
    j.at("w").get_to(rect.w);
    j.at("h").get_to(rect.h);
}

struct AnimationFrame {
    SDL_Rect sourceRect; 
    float duration;
};

inline void to_json(nlohmann::json& j, const AnimationFrame& frame) {
    j = nlohmann::json{
        {"sourceRect", frame.sourceRect},
        {"duration", frame.duration}
    };
}

inline void from_json(const nlohmann::json& j, AnimationFrame& frame) {
    j.at("sourceRect").get_to(frame.sourceRect);
    j.at("duration").get_to(frame.duration);
}

struct AnimationSequence {
    std::string name;
    std::string textureId; 
    std::vector<AnimationFrame> frames;
    bool loop = false;
};

inline void to_json(nlohmann::json& j, const AnimationSequence& seq) {
    j = nlohmann::json{
        {"name", seq.name},
        {"textureId", seq.textureId},
        {"frames", seq.frames},
        {"loop", seq.loop}
    };
}

inline void from_json(const nlohmann::json& j, AnimationSequence& seq) {
    j.at("name").get_to(seq.name);
    j.at("textureId").get_to(seq.textureId);
    j.at("frames").get_to(seq.frames);
    j.at("loop").get_to(seq.loop);
}