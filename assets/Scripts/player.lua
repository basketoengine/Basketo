-- player.lua
-- This script controls the player entity

local playerSpeed = 150.0 -- units per second, adjust as needed

function init(entity)
    Log("Player script initialized for entity: " .. tostring(entity))
    local pos = GetEntityPosition(entity)
    if pos then
        Log("Player initial position: x=" .. pos[1] .. ", y=" .. pos[2])
    else
        LogError("Could not get initial position for player entity in Lua.")
    end
end

function update(entity, deltaTime)
    local vx, vy = 0, 0
    local speed = 150
    if Input.isKeyDown("W") then vy = vy - speed end
    if Input.isKeyDown("S") then vy = vy + speed end
    if Input.isKeyDown("A") then vx = vx - speed end
    if Input.isKeyDown("D") then vx = vx + speed end
    SetEntityVelocity(entity, vx, vy)
end

-- You can add more functions here for other events later, e.g.:
-- function onCollision(entity, otherEntity, collisionDetails)
--     Log("Player entity " .. tostring(entity) .. " collided with " .. tostring(otherEntity))
-- end
