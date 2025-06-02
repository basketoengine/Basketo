-- Enhanced Player Script with Sound Effects
-- This script demonstrates how to use the new SoundEffectsComponent

local entity = ...
local jumpForce = 400
local moveSpeed = 200
local wasGrounded = true
local isMoving = false

function init(entityId)
    entity = entityId
    Log("Player with sounds initialized for entity: " .. tostring(entity))
    
    -- Check if entity has SoundEffectsComponent
    if HasEntityComponent(entity, "SoundEffectsComponent") then
        Log("Entity has SoundEffectsComponent - sound effects enabled!")
    else
        Log("Entity does not have SoundEffectsComponent - add one in the inspector for sound effects")
    end
end

function update(entityId, deltaTime)
    entity = entityId
    
    -- Get current state
    local velocity = GetEntityVelocity(entity)
    if not velocity then
        LogError("Failed to get velocity for entity: " .. tostring(entity))
        return
    end
    
    local vx, vy = velocity[1], velocity[2]
    local contacts = GetCollisionContacts(entity)
    local grounded = false
    
    -- Check if grounded (touching something below)
    if contacts then
        for i, contact in pairs(contacts) do
            if contact.normalY and contact.normalY < -0.5 then
                grounded = true
                break
            end
        end
    end
    
    -- Landing sound effect
    if grounded and not wasGrounded and vy > 50 then
        PlaySound(entity, "land")
    end
    
    -- Movement input and sound effects
    local newVx = vx
    local currentlyMoving = false
    
    if Input.isKeyDown("A") or Input.isKeyDown("Left") then
        newVx = -moveSpeed
        currentlyMoving = true
        SetEntityFlipHorizontal(entity, true)
        
        -- Play walking sound if just started moving or if grounded
        if grounded and (not isMoving or math.abs(vx) < 50) then
            PlaySound(entity, "walk")
        end
    elseif Input.isKeyDown("D") or Input.isKeyDown("Right") then
        newVx = moveSpeed
        currentlyMoving = true
        SetEntityFlipHorizontal(entity, false)
        
        -- Play walking sound if just started moving or if grounded
        if grounded and (not isMoving or math.abs(vx) < 50) then
            PlaySound(entity, "walk")
        end
    else
        newVx = 0
        currentlyMoving = false
    end
    
    -- Jumping
    if (Input.isKeyDown("W") or Input.isKeyDown("Space")) and grounded then
        vy = -jumpForce
        PlaySound(entity, "jump")
        Log("Jump! Playing jump sound")
    end
    
    -- Update animation based on movement
    if grounded then
        if currentlyMoving then
            SetEntityAnimation(entity, "running")
        else
            SetEntityAnimation(entity, "idle")
        end
    else
        SetEntityAnimation(entity, "jumping")
    end
    
    -- Apply velocity
    SetEntityVelocity(entity, newVx, vy)
    
    -- Update state tracking
    wasGrounded = grounded
    isMoving = currentlyMoving
    
    -- Debug: Show sound effects available (uncomment for testing)
    -- if HasSoundEffect(entity, "jump") then
    --     Log("Jump sound effect is available")
    -- end
end
