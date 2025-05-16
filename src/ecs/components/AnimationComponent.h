#pragma once
#include <string>
#include <vector>
#include <map>
#include "../../animation/Animation.h" // Or wherever AnimationSequence is defined
#include "../../vendor/nlohmann/json.hpp"

struct AnimationComponent {
    std::map<std::string, AnimationSequence> animations;
    std::string currentAnimationName;
    int currentFrameIndex = 0;
    float currentFrameTime = 0.0f;
    bool isPlaying = false;
    bool flipHorizontal = false; // Optional: for flipping sprites
    bool flipVertical = false;   // Optional: for flipping sprites

    void addAnimation(const AnimationSequence& sequence) {
        animations[sequence.name] = sequence;
    }

    bool play(const std::string& name, bool forceRestart = false) {
        if (animations.count(name)) {
            if (currentAnimationName == name && isPlaying && !forceRestart) {
                return true; // Already playing this animation
            }
            currentAnimationName = name;
            currentFrameIndex = 0;
            currentFrameTime = 0.0f;
            isPlaying = true;
            // Potentially update sprite's texture ID here if animations can use different textures
            // and your SpriteComponent needs it.
            return true;
        }
        return false; // Animation not found
    }

    void stop() {
        isPlaying = false;
        currentFrameIndex = 0; // Or keep last frame
        currentFrameTime = 0.0f;
    }
};

// JSON serialization for AnimationComponent
inline void to_json(nlohmann::json& j, const AnimationComponent& comp) {
    j = nlohmann::json{
        {"animations", comp.animations},
        {"currentAnimationName", comp.currentAnimationName},
        {"currentFrameIndex", comp.currentFrameIndex},
        {"currentFrameTime", comp.currentFrameTime},
        {"isPlaying", comp.isPlaying},
        {"flipHorizontal", comp.flipHorizontal},
        {"flipVertical", comp.flipVertical}
    };
}

inline void from_json(const nlohmann::json& j, AnimationComponent& comp) {
    j.at("animations").get_to(comp.animations);
    j.at("currentAnimationName").get_to(comp.currentAnimationName);
    j.at("currentFrameIndex").get_to(comp.currentFrameIndex);
    j.at("currentFrameTime").get_to(comp.currentFrameTime);
    j.at("isPlaying").get_to(comp.isPlaying);
    j.at("flipHorizontal").get_to(comp.flipHorizontal);
    j.at("flipVertical").get_to(comp.flipVertical);
}

