-- Particle System Demo Script
-- This script demonstrates various particle effects

local effectTimer = 0.0
local currentEffect = 1
local effectDuration = 3.0  -- Switch effects every 3 seconds
local effects = {"fire", "explosion", "smoke", "sparkle"}

function init(entity)
    Log("Particle Demo script initialized for entity: " .. tostring(entity))
    
    -- Start with fire effect
    CreateFireEffect(entity)
    Log("Created initial fire effect")
end

function update(entity, deltaTime)
    effectTimer = effectTimer + deltaTime
    
    -- Switch between different particle effects
    if effectTimer >= effectDuration then
        effectTimer = 0.0
        currentEffect = currentEffect + 1
        if currentEffect > #effects then
            currentEffect = 1
        end
        
        local effectName = effects[currentEffect]
        Log("Switching to " .. effectName .. " effect")
        
        -- Create the appropriate effect
        if effectName == "fire" then
            CreateFireEffect(entity)
        elseif effectName == "explosion" then
            CreateExplosionEffect(entity)
        elseif effectName == "smoke" then
            CreateSmokeEffect(entity)
        elseif effectName == "sparkle" then
            CreateSparkleEffect(entity)
        end
    end
    
    -- Log particle count every 2 seconds
    if math.floor(effectTimer * 10) % 20 == 0 then
        local particleCount = GetActiveParticleCount(entity)
        Log("Active particles: " .. tostring(particleCount))
    end
    
    -- Handle input for manual effect switching
    if Input.isKeyDown("1") then
        CreateFireEffect(entity)
        Log("Manual fire effect created")
    elseif Input.isKeyDown("2") then
        CreateExplosionEffect(entity)
        Log("Manual explosion effect created")
    elseif Input.isKeyDown("3") then
        CreateSmokeEffect(entity)
        Log("Manual smoke effect created")
    elseif Input.isKeyDown("4") then
        CreateSparkleEffect(entity)
        Log("Manual sparkle effect created")
    elseif Input.isKeyDown("5") then
        -- Toggle emitter on/off
        EnableParticleEmitter(entity, false)
        Log("Particle emitter disabled")
    elseif Input.isKeyDown("6") then
        EnableParticleEmitter(entity, true)
        Log("Particle emitter enabled")
    elseif Input.isKeyDown("7") then
        -- Increase emission rate
        SetParticleEmissionRate(entity, 50.0)
        Log("Increased emission rate to 50")
    elseif Input.isKeyDown("8") then
        -- Decrease emission rate
        SetParticleEmissionRate(entity, 10.0)
        Log("Decreased emission rate to 10")
    end
end
