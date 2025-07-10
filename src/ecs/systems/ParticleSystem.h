#pragma once

#include "../System.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/ParticleComponent.h"
#include "../../AssetManager.h"
#include <SDL2/SDL.h>
#include <random>
#include <cmath>
#include <iostream>

class ParticleSystem : public System {
public:
    ParticleSystem();
    ~ParticleSystem() = default;
    
    void update(ComponentManager* componentManager, float deltaTime);
    void render(SDL_Renderer* renderer, ComponentManager* componentManager, float cameraX, float cameraY);
    
    // Performance metrics
    struct PerformanceMetrics {
        int totalParticles = 0;
        int activeParticles = 0;
        int particlesEmittedThisFrame = 0;
        int particlesKilledThisFrame = 0;
        float updateTime = 0.0f;
        float renderTime = 0.0f;
    };
    
    const PerformanceMetrics& getMetrics() const { return metrics; }
    void resetMetrics();
    
    // Particle effect presets
    void createFireEffect(Entity entity, ComponentManager* componentManager);
    void createExplosionEffect(Entity entity, ComponentManager* componentManager);
    void createSmokeEffect(Entity entity, ComponentManager* componentManager);
    void createSparkleEffect(Entity entity, ComponentManager* componentManager);
    void createRainEffect(Entity entity, ComponentManager* componentManager);
    
private:
    AssetManager& assetManager;
    std::mt19937 randomGenerator;
    std::uniform_real_distribution<float> uniformDist;
    PerformanceMetrics metrics;
    
    // Helper methods
    void updateEmitter(Entity entity, ComponentManager* componentManager, float deltaTime);
    void updateParticles(Entity entity, ComponentManager* componentManager, float deltaTime);
    void emitParticle(Entity entity, ComponentManager* componentManager);
    void initializeParticle(Particle& particle, const ParticleEmitterComponent& emitter, 
                           const TransformComponent& transform);
    
    // Emission shape helpers
    void getEmissionPosition(const ParticleEmitterComponent& emitter, 
                           const TransformComponent& transform, float& x, float& y);
    void getEmissionVelocity(const ParticleEmitterComponent& emitter, float& vx, float& vy);
    
    // Rendering helpers
    void setBlendMode(SDL_Renderer* renderer, ParticleBlendMode blendMode);
    void renderParticle(SDL_Renderer* renderer, const Particle& particle, 
                       SDL_Texture* texture, float cameraX, float cameraY);
    
    // Utility functions
    float randomFloat(float min, float max);
    float degreesToRadians(float degrees);
};

// Particle effect factory functions
namespace ParticleEffects {
    ParticleEmitterComponent createFireEmitter();
    ParticleEmitterComponent createExplosionEmitter();
    ParticleEmitterComponent createSmokeEmitter();
    ParticleEmitterComponent createSparkleEmitter();
    ParticleEmitterComponent createRainEmitter();
}
