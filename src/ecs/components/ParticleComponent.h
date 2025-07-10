#pragma once

#include <vector>
#include <string>
#include <SDL2/SDL.h>
#include "../../../vendor/nlohmann/json.hpp"

// Individual particle data
struct Particle {
    float x, y;                    // Position
    float vx, vy;                  // Velocity
    float ax, ay;                  // Acceleration
    float life;                    // Current life (0.0 to maxLife)
    float maxLife;                 // Maximum lifetime
    float size;                    // Current size
    float rotation;                // Current rotation
    float rotationSpeed;           // Rotation speed
    SDL_Color color;               // Current color
    bool active;                   // Is particle active
    
    Particle() : x(0), y(0), vx(0), vy(0), ax(0), ay(0), 
                 life(0), maxLife(1.0f), size(1.0f), rotation(0), rotationSpeed(0),
                 color{255, 255, 255, 255}, active(false) {}
};

// Emission shapes
enum class EmissionShape {
    POINT,
    CIRCLE,
    RECTANGLE,
    LINE
};

// Blend modes for particle rendering
enum class ParticleBlendMode {
    ALPHA,
    ADDITIVE,
    MULTIPLY
};

// Color curve point for animating color over lifetime
struct ColorCurvePoint {
    float time;        // 0.0 to 1.0 (lifetime percentage)
    SDL_Color color;
    
    ColorCurvePoint(float t = 0.0f, SDL_Color c = {255, 255, 255, 255}) 
        : time(t), color(c) {}
};

// Size curve point for animating size over lifetime
struct SizeCurvePoint {
    float time;        // 0.0 to 1.0 (lifetime percentage)
    float size;
    
    SizeCurvePoint(float t = 0.0f, float s = 1.0f) : time(t), size(s) {}
};

// Particle emitter configuration
struct ParticleEmitterComponent {
    // Basic emission properties
    bool enabled = true;
    float emissionRate = 10.0f;        // Particles per second
    int maxParticles = 100;            // Maximum number of particles
    float emissionTimer = 0.0f;        // Internal timer for emission
    bool looping = true;               // Should the emitter loop
    float duration = 5.0f;             // Duration before stopping (if not looping)
    float emissionTime = 0.0f;         // Time since emission started
    
    // Particle lifetime
    float minLifetime = 1.0f;
    float maxLifetime = 3.0f;
    
    // Emission shape and area
    EmissionShape shape = EmissionShape::POINT;
    float shapeRadius = 10.0f;         // For circle
    float shapeWidth = 20.0f;          // For rectangle/line
    float shapeHeight = 20.0f;         // For rectangle
    
    // Initial velocity
    float minSpeed = 50.0f;
    float maxSpeed = 100.0f;
    float directionAngle = 0.0f;       // Base direction in degrees
    float directionSpread = 45.0f;     // Spread angle in degrees
    
    // Physics
    float gravityX = 0.0f;
    float gravityY = 98.0f;            // Default gravity
    float damping = 0.98f;             // Velocity damping (0.0 to 1.0)
    
    // Visual properties
    std::string textureId = "";        // Texture for particles
    ParticleBlendMode blendMode = ParticleBlendMode::ALPHA;
    
    // Size animation
    float minStartSize = 1.0f;
    float maxStartSize = 1.0f;
    std::vector<SizeCurvePoint> sizeCurve;
    
    // Color animation
    SDL_Color startColor = {255, 255, 255, 255};
    SDL_Color endColor = {255, 255, 255, 0};
    std::vector<ColorCurvePoint> colorCurve;
    
    // Rotation
    float minStartRotation = 0.0f;
    float maxStartRotation = 0.0f;
    float minRotationSpeed = 0.0f;
    float maxRotationSpeed = 0.0f;
    
    ParticleEmitterComponent() {
        // Default size curve (start big, end small)
        sizeCurve.push_back(SizeCurvePoint(0.0f, 1.0f));
        sizeCurve.push_back(SizeCurvePoint(1.0f, 0.0f));
        
        // Default color curve (fade out)
        colorCurve.push_back(ColorCurvePoint(0.0f, {255, 255, 255, 255}));
        colorCurve.push_back(ColorCurvePoint(1.0f, {255, 255, 255, 0}));
    }
    
    // Helper methods
    SDL_Color interpolateColor(float t) const;
    float interpolateSize(float t) const;
    void resetEmission();
};

// Component that holds the actual particles
struct ParticleComponent {
    std::vector<Particle> particles;
    int activeParticleCount = 0;
    
    // Performance tracking
    float lastUpdateTime = 0.0f;
    int particlesEmittedThisFrame = 0;
    
    ParticleComponent() = default;
    
    void reserveParticles(int count) {
        particles.reserve(count);
        particles.resize(count);
    }
    
    Particle* getInactiveParticle() {
        for (auto& particle : particles) {
            if (!particle.active) {
                return &particle;
            }
        }
        return nullptr;
    }
    
    void updateActiveCount() {
        activeParticleCount = 0;
        for (const auto& particle : particles) {
            if (particle.active) {
                activeParticleCount++;
            }
        }
    }
};

// JSON serialization for ParticleEmitterComponent
inline void to_json(nlohmann::json& j, const ParticleEmitterComponent& comp) {
    j = nlohmann::json{
        {"enabled", comp.enabled},
        {"emissionRate", comp.emissionRate},
        {"maxParticles", comp.maxParticles},
        {"looping", comp.looping},
        {"duration", comp.duration},
        {"minLifetime", comp.minLifetime},
        {"maxLifetime", comp.maxLifetime},
        {"shape", static_cast<int>(comp.shape)},
        {"shapeRadius", comp.shapeRadius},
        {"shapeWidth", comp.shapeWidth},
        {"shapeHeight", comp.shapeHeight},
        {"minSpeed", comp.minSpeed},
        {"maxSpeed", comp.maxSpeed},
        {"directionAngle", comp.directionAngle},
        {"directionSpread", comp.directionSpread},
        {"gravityX", comp.gravityX},
        {"gravityY", comp.gravityY},
        {"damping", comp.damping},
        {"textureId", comp.textureId},
        {"blendMode", static_cast<int>(comp.blendMode)},
        {"minStartSize", comp.minStartSize},
        {"maxStartSize", comp.maxStartSize},
        {"startColor", {comp.startColor.r, comp.startColor.g, comp.startColor.b, comp.startColor.a}},
        {"endColor", {comp.endColor.r, comp.endColor.g, comp.endColor.b, comp.endColor.a}},
        {"minStartRotation", comp.minStartRotation},
        {"maxStartRotation", comp.maxStartRotation},
        {"minRotationSpeed", comp.minRotationSpeed},
        {"maxRotationSpeed", comp.maxRotationSpeed}
    };
}

inline void from_json(const nlohmann::json& j, ParticleEmitterComponent& comp) {
    comp.enabled = j.value("enabled", true);
    comp.emissionRate = j.value("emissionRate", 10.0f);
    comp.maxParticles = j.value("maxParticles", 100);
    comp.looping = j.value("looping", true);
    comp.duration = j.value("duration", 5.0f);
    comp.minLifetime = j.value("minLifetime", 1.0f);
    comp.maxLifetime = j.value("maxLifetime", 3.0f);
    comp.shape = static_cast<EmissionShape>(j.value("shape", 0));
    comp.shapeRadius = j.value("shapeRadius", 10.0f);
    comp.shapeWidth = j.value("shapeWidth", 20.0f);
    comp.shapeHeight = j.value("shapeHeight", 20.0f);
    comp.minSpeed = j.value("minSpeed", 50.0f);
    comp.maxSpeed = j.value("maxSpeed", 100.0f);
    comp.directionAngle = j.value("directionAngle", 0.0f);
    comp.directionSpread = j.value("directionSpread", 45.0f);
    comp.gravityX = j.value("gravityX", 0.0f);
    comp.gravityY = j.value("gravityY", 98.0f);
    comp.damping = j.value("damping", 0.98f);
    comp.textureId = j.value("textureId", "");
    comp.blendMode = static_cast<ParticleBlendMode>(j.value("blendMode", 0));
    comp.minStartSize = j.value("minStartSize", 1.0f);
    comp.maxStartSize = j.value("maxStartSize", 1.0f);

    if (j.contains("startColor")) {
        auto sc = j["startColor"];
        comp.startColor = {sc[0], sc[1], sc[2], sc[3]};
    }
    if (j.contains("endColor")) {
        auto ec = j["endColor"];
        comp.endColor = {ec[0], ec[1], ec[2], ec[3]};
    }

    comp.minStartRotation = j.value("minStartRotation", 0.0f);
    comp.maxStartRotation = j.value("maxStartRotation", 0.0f);
    comp.minRotationSpeed = j.value("minRotationSpeed", 0.0f);
    comp.maxRotationSpeed = j.value("maxRotationSpeed", 0.0f);
}
