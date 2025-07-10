#include "ParticleSystem.h"
#include <chrono>
#include <algorithm>

ParticleSystem::ParticleSystem() 
    : assetManager(AssetManager::getInstance()), 
      randomGenerator(std::chrono::steady_clock::now().time_since_epoch().count()),
      uniformDist(0.0f, 1.0f) {
    std::cout << "[ParticleSystem] Initialized" << std::endl;
}

void ParticleSystem::update(ComponentManager* componentManager, float deltaTime) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    metrics.particlesEmittedThisFrame = 0;
    metrics.particlesKilledThisFrame = 0;
    metrics.totalParticles = 0;
    metrics.activeParticles = 0;
    
    for (auto const& entity : entities) {
        if (componentManager->hasComponent<ParticleEmitterComponent>(entity) &&
            componentManager->hasComponent<ParticleComponent>(entity) &&
            componentManager->hasComponent<TransformComponent>(entity)) {
            
            updateEmitter(entity, componentManager, deltaTime);
            updateParticles(entity, componentManager, deltaTime);
            
            auto& particleComp = componentManager->getComponent<ParticleComponent>(entity);
            metrics.totalParticles += static_cast<int>(particleComp.particles.size());
            metrics.activeParticles += particleComp.activeParticleCount;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    metrics.updateTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

void ParticleSystem::updateEmitter(Entity entity, ComponentManager* componentManager, float deltaTime) {
    auto& emitter = componentManager->getComponent<ParticleEmitterComponent>(entity);
    auto& particleComp = componentManager->getComponent<ParticleComponent>(entity);
    
    if (!emitter.enabled) return;
    
    // Update emission timing
    emitter.emissionTime += deltaTime;
    
    // Check if emitter should stop (non-looping)
    if (!emitter.looping && emitter.emissionTime >= emitter.duration) {
        emitter.enabled = false;
        return;
    }
    
    // Ensure particle storage is adequate
    if (particleComp.particles.size() < static_cast<size_t>(emitter.maxParticles)) {
        particleComp.reserveParticles(emitter.maxParticles);
    }
    
    // Emit particles based on emission rate
    emitter.emissionTimer += deltaTime;
    float emissionInterval = 1.0f / emitter.emissionRate;
    
    while (emitter.emissionTimer >= emissionInterval && 
           particleComp.activeParticleCount < emitter.maxParticles) {
        emitParticle(entity, componentManager);
        emitter.emissionTimer -= emissionInterval;
        metrics.particlesEmittedThisFrame++;
    }
}

void ParticleSystem::updateParticles(Entity entity, ComponentManager* componentManager, float deltaTime) {
    auto& emitter = componentManager->getComponent<ParticleEmitterComponent>(entity);
    auto& particleComp = componentManager->getComponent<ParticleComponent>(entity);
    
    for (auto& particle : particleComp.particles) {
        if (!particle.active) continue;
        
        // Update lifetime
        particle.life += deltaTime;
        if (particle.life >= particle.maxLife) {
            particle.active = false;
            metrics.particlesKilledThisFrame++;
            continue;
        }
        
        // Calculate normalized lifetime (0.0 to 1.0)
        float t = particle.life / particle.maxLife;
        
        // Update visual properties based on curves
        particle.color = emitter.interpolateColor(t);
        particle.size = emitter.interpolateSize(t);
        
        // Update physics
        particle.ax = emitter.gravityX;
        particle.ay = emitter.gravityY;
        
        particle.vx += particle.ax * deltaTime;
        particle.vy += particle.ay * deltaTime;
        
        // Apply damping
        particle.vx *= emitter.damping;
        particle.vy *= emitter.damping;
        
        // Update position
        particle.x += particle.vx * deltaTime;
        particle.y += particle.vy * deltaTime;
        
        // Update rotation
        particle.rotation += particle.rotationSpeed * deltaTime;
    }
    
    // Update active particle count
    particleComp.updateActiveCount();
}

void ParticleSystem::emitParticle(Entity entity, ComponentManager* componentManager) {
    auto& emitter = componentManager->getComponent<ParticleEmitterComponent>(entity);
    auto& particleComp = componentManager->getComponent<ParticleComponent>(entity);
    auto& transform = componentManager->getComponent<TransformComponent>(entity);
    
    Particle* particle = particleComp.getInactiveParticle();
    if (!particle) return;
    
    initializeParticle(*particle, emitter, transform);
    particle->active = true;
}

void ParticleSystem::initializeParticle(Particle& particle, const ParticleEmitterComponent& emitter, 
                                       const TransformComponent& transform) {
    // Set position based on emission shape
    getEmissionPosition(emitter, transform, particle.x, particle.y);
    
    // Set velocity
    getEmissionVelocity(emitter, particle.vx, particle.vy);
    
    // Set lifetime
    particle.maxLife = randomFloat(emitter.minLifetime, emitter.maxLifetime);
    particle.life = 0.0f;
    
    // Set initial size
    particle.size = randomFloat(emitter.minStartSize, emitter.maxStartSize);
    
    // Set initial rotation
    particle.rotation = randomFloat(emitter.minStartRotation, emitter.maxStartRotation);
    particle.rotationSpeed = randomFloat(emitter.minRotationSpeed, emitter.maxRotationSpeed);
    
    // Set initial color
    particle.color = emitter.startColor;
    
    // Reset acceleration
    particle.ax = 0.0f;
    particle.ay = 0.0f;
}

void ParticleSystem::getEmissionPosition(const ParticleEmitterComponent& emitter, 
                                        const TransformComponent& transform, float& x, float& y) {
    x = transform.x + transform.width * 0.5f;
    y = transform.y + transform.height * 0.5f;
    
    switch (emitter.shape) {
        case EmissionShape::POINT:
            // Already set to center
            break;
            
        case EmissionShape::CIRCLE: {
            float angle = randomFloat(0.0f, 2.0f * M_PI);
            float radius = randomFloat(0.0f, emitter.shapeRadius);
            x += cos(angle) * radius;
            y += sin(angle) * radius;
            break;
        }
        
        case EmissionShape::RECTANGLE: {
            x += randomFloat(-emitter.shapeWidth * 0.5f, emitter.shapeWidth * 0.5f);
            y += randomFloat(-emitter.shapeHeight * 0.5f, emitter.shapeHeight * 0.5f);
            break;
        }
        
        case EmissionShape::LINE: {
            x += randomFloat(-emitter.shapeWidth * 0.5f, emitter.shapeWidth * 0.5f);
            // y stays at center for horizontal line
            break;
        }
    }
}

void ParticleSystem::getEmissionVelocity(const ParticleEmitterComponent& emitter, float& vx, float& vy) {
    float speed = randomFloat(emitter.minSpeed, emitter.maxSpeed);
    float baseAngle = degreesToRadians(emitter.directionAngle);
    float spread = degreesToRadians(emitter.directionSpread);
    float angle = baseAngle + randomFloat(-spread * 0.5f, spread * 0.5f);
    
    vx = cos(angle) * speed;
    vy = sin(angle) * speed;
}

float ParticleSystem::randomFloat(float min, float max) {
    return min + uniformDist(randomGenerator) * (max - min);
}

float ParticleSystem::degreesToRadians(float degrees) {
    return degrees * M_PI / 180.0f;
}

void ParticleSystem::resetMetrics() {
    metrics = PerformanceMetrics{};
}

void ParticleSystem::render(SDL_Renderer* renderer, ComponentManager* componentManager, float cameraX, float cameraY) {
    auto startTime = std::chrono::high_resolution_clock::now();

    for (auto const& entity : entities) {
        if (!componentManager->hasComponent<ParticleEmitterComponent>(entity) ||
            !componentManager->hasComponent<ParticleComponent>(entity)) {
            continue;
        }

        auto& emitter = componentManager->getComponent<ParticleEmitterComponent>(entity);
        auto& particleComp = componentManager->getComponent<ParticleComponent>(entity);

        // Get texture if specified
        SDL_Texture* texture = nullptr;
        if (!emitter.textureId.empty()) {
            texture = assetManager.getTexture(emitter.textureId);
        }

        // Set blend mode
        setBlendMode(renderer, emitter.blendMode);

        // Render all active particles
        for (const auto& particle : particleComp.particles) {
            if (particle.active) {
                renderParticle(renderer, particle, texture, cameraX, cameraY);
            }
        }

        // Reset blend mode to default
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    metrics.renderTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

void ParticleSystem::setBlendMode(SDL_Renderer* renderer, ParticleBlendMode blendMode) {
    switch (blendMode) {
        case ParticleBlendMode::ALPHA:
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            break;
        case ParticleBlendMode::ADDITIVE:
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            break;
        case ParticleBlendMode::MULTIPLY:
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MOD);
            break;
    }
}

void ParticleSystem::renderParticle(SDL_Renderer* renderer, const Particle& particle,
                                   SDL_Texture* texture, float cameraX, float cameraY) {
    int screenX = static_cast<int>(particle.x - cameraX);
    int screenY = static_cast<int>(particle.y - cameraY);
    int size = static_cast<int>(particle.size);

    if (texture) {
        // Render textured particle
        SDL_Rect destRect = {
            screenX - size / 2,
            screenY - size / 2,
            size,
            size
        };

        // Set texture color modulation
        SDL_SetTextureColorMod(texture, particle.color.r, particle.color.g, particle.color.b);
        SDL_SetTextureAlphaMod(texture, particle.color.a);

        // Render with rotation if needed
        if (particle.rotation != 0.0f) {
            SDL_RenderCopyEx(renderer, texture, nullptr, &destRect,
                           particle.rotation * 180.0f / M_PI, nullptr, SDL_FLIP_NONE);
        } else {
            SDL_RenderCopy(renderer, texture, nullptr, &destRect);
        }
    } else {
        // Render as colored rectangle
        SDL_SetRenderDrawColor(renderer, particle.color.r, particle.color.g,
                              particle.color.b, particle.color.a);

        SDL_Rect rect = {
            screenX - size / 2,
            screenY - size / 2,
            size,
            size
        };

        SDL_RenderFillRect(renderer, &rect);
    }
}

// Particle effect presets
void ParticleSystem::createFireEffect(Entity entity, ComponentManager* componentManager) {
    auto emitter = ParticleEffects::createFireEmitter();
    componentManager->addComponent(entity, emitter);

    ParticleComponent particleComp;
    particleComp.reserveParticles(emitter.maxParticles);
    componentManager->addComponent(entity, particleComp);
}

void ParticleSystem::createExplosionEffect(Entity entity, ComponentManager* componentManager) {
    auto emitter = ParticleEffects::createExplosionEmitter();
    componentManager->addComponent(entity, emitter);

    ParticleComponent particleComp;
    particleComp.reserveParticles(emitter.maxParticles);
    componentManager->addComponent(entity, particleComp);
}

void ParticleSystem::createSmokeEffect(Entity entity, ComponentManager* componentManager) {
    auto emitter = ParticleEffects::createSmokeEmitter();
    componentManager->addComponent(entity, emitter);

    ParticleComponent particleComp;
    particleComp.reserveParticles(emitter.maxParticles);
    componentManager->addComponent(entity, particleComp);
}

void ParticleSystem::createSparkleEffect(Entity entity, ComponentManager* componentManager) {
    auto emitter = ParticleEffects::createSparkleEmitter();
    componentManager->addComponent(entity, emitter);

    ParticleComponent particleComp;
    particleComp.reserveParticles(emitter.maxParticles);
    componentManager->addComponent(entity, particleComp);
}

void ParticleSystem::createRainEffect(Entity entity, ComponentManager* componentManager) {
    auto emitter = ParticleEffects::createRainEmitter();
    componentManager->addComponent(entity, emitter);

    ParticleComponent particleComp;
    particleComp.reserveParticles(emitter.maxParticles);
    componentManager->addComponent(entity, particleComp);
}

// Particle effect factory implementations
namespace ParticleEffects {
    ParticleEmitterComponent createFireEmitter() {
        ParticleEmitterComponent emitter;
        emitter.emissionRate = 30.0f;
        emitter.maxParticles = 150;
        emitter.minLifetime = 0.5f;
        emitter.maxLifetime = 1.5f;
        emitter.shape = EmissionShape::CIRCLE;
        emitter.shapeRadius = 5.0f;
        emitter.minSpeed = 20.0f;
        emitter.maxSpeed = 60.0f;
        emitter.directionAngle = -90.0f; // Upward
        emitter.directionSpread = 30.0f;
        emitter.gravityY = -20.0f; // Slight upward force
        emitter.damping = 0.95f;
        emitter.blendMode = ParticleBlendMode::ADDITIVE;
        emitter.minStartSize = 2.0f;
        emitter.maxStartSize = 6.0f;
        emitter.startColor = {255, 100, 0, 255}; // Orange
        emitter.endColor = {255, 0, 0, 0}; // Red fading to transparent

        // Fire-like size curve (start small, grow, then shrink)
        emitter.sizeCurve.clear();
        emitter.sizeCurve.push_back(SizeCurvePoint(0.0f, 0.3f));
        emitter.sizeCurve.push_back(SizeCurvePoint(0.3f, 1.0f));
        emitter.sizeCurve.push_back(SizeCurvePoint(1.0f, 0.1f));

        return emitter;
    }

    ParticleEmitterComponent createExplosionEmitter() {
        ParticleEmitterComponent emitter;
        emitter.emissionRate = 200.0f;
        emitter.maxParticles = 100;
        emitter.minLifetime = 0.3f;
        emitter.maxLifetime = 1.0f;
        emitter.shape = EmissionShape::POINT;
        emitter.minSpeed = 100.0f;
        emitter.maxSpeed = 300.0f;
        emitter.directionAngle = 0.0f;
        emitter.directionSpread = 360.0f; // All directions
        emitter.gravityY = 200.0f; // Strong downward gravity
        emitter.damping = 0.9f;
        emitter.blendMode = ParticleBlendMode::ADDITIVE;
        emitter.minStartSize = 3.0f;
        emitter.maxStartSize = 8.0f;
        emitter.startColor = {255, 255, 100, 255}; // Bright yellow
        emitter.endColor = {255, 50, 0, 0}; // Orange fading out
        emitter.looping = false;
        emitter.duration = 0.2f; // Short burst

        return emitter;
    }

    ParticleEmitterComponent createSmokeEmitter() {
        ParticleEmitterComponent emitter;
        emitter.emissionRate = 15.0f;
        emitter.maxParticles = 80;
        emitter.minLifetime = 2.0f;
        emitter.maxLifetime = 4.0f;
        emitter.shape = EmissionShape::CIRCLE;
        emitter.shapeRadius = 8.0f;
        emitter.minSpeed = 10.0f;
        emitter.maxSpeed = 30.0f;
        emitter.directionAngle = -90.0f; // Upward
        emitter.directionSpread = 20.0f;
        emitter.gravityY = -10.0f; // Slight upward drift
        emitter.damping = 0.98f;
        emitter.blendMode = ParticleBlendMode::ALPHA;
        emitter.minStartSize = 4.0f;
        emitter.maxStartSize = 8.0f;
        emitter.startColor = {100, 100, 100, 150}; // Gray
        emitter.endColor = {200, 200, 200, 0}; // Light gray fading out

        // Smoke grows over time
        emitter.sizeCurve.clear();
        emitter.sizeCurve.push_back(SizeCurvePoint(0.0f, 0.5f));
        emitter.sizeCurve.push_back(SizeCurvePoint(1.0f, 2.0f));

        return emitter;
    }

    ParticleEmitterComponent createSparkleEmitter() {
        ParticleEmitterComponent emitter;
        emitter.emissionRate = 20.0f;
        emitter.maxParticles = 60;
        emitter.minLifetime = 1.0f;
        emitter.maxLifetime = 2.0f;
        emitter.shape = EmissionShape::CIRCLE;
        emitter.shapeRadius = 15.0f;
        emitter.minSpeed = 5.0f;
        emitter.maxSpeed = 25.0f;
        emitter.directionAngle = 0.0f;
        emitter.directionSpread = 360.0f;
        emitter.gravityY = 0.0f; // No gravity
        emitter.damping = 0.99f;
        emitter.blendMode = ParticleBlendMode::ADDITIVE;
        emitter.minStartSize = 1.0f;
        emitter.maxStartSize = 3.0f;
        emitter.startColor = {255, 255, 255, 255}; // White
        emitter.endColor = {255, 255, 100, 0}; // Yellow fading out
        emitter.minRotationSpeed = -180.0f;
        emitter.maxRotationSpeed = 180.0f;

        return emitter;
    }

    ParticleEmitterComponent createRainEmitter() {
        ParticleEmitterComponent emitter;
        emitter.emissionRate = 100.0f;
        emitter.maxParticles = 300;
        emitter.minLifetime = 2.0f;
        emitter.maxLifetime = 3.0f;
        emitter.shape = EmissionShape::RECTANGLE;
        emitter.shapeWidth = 200.0f;
        emitter.shapeHeight = 10.0f;
        emitter.minSpeed = 200.0f;
        emitter.maxSpeed = 250.0f;
        emitter.directionAngle = 90.0f; // Downward
        emitter.directionSpread = 5.0f;
        emitter.gravityY = 300.0f; // Strong downward gravity
        emitter.damping = 1.0f; // No damping
        emitter.blendMode = ParticleBlendMode::ALPHA;
        emitter.minStartSize = 1.0f;
        emitter.maxStartSize = 2.0f;
        emitter.startColor = {100, 150, 255, 200}; // Blue
        emitter.endColor = {100, 150, 255, 200}; // Constant color

        return emitter;
    }
}
