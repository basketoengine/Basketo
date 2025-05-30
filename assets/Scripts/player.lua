local playerSpeed = 150.0 

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
    Log("Player Lua update function called. Entity: " .. tostring(entity) .. ", DeltaTime: " .. tostring(deltaTime))

    local vx, vy = 0, 0

    local w_pressed = Input.isKeyDown("W")
    local s_pressed = Input.isKeyDown("S")
    local a_pressed = Input.isKeyDown("A")
    local d_pressed = Input.isKeyDown("D")
    Log("Lua update: W=" .. tostring(w_pressed) .. " A=" .. tostring(a_pressed) .. " S=" .. tostring(s_pressed) .. " D=" .. tostring(d_pressed))

    if w_pressed then vy = vy - playerSpeed end
    if s_pressed then vy = vy + playerSpeed end
    if a_pressed then vx = vx - playerSpeed end
    if d_pressed then vx = vx + playerSpeed end

    SetEntityVelocity(entity, vx, vy)
    -- Log("Lua set velocity: vx=" .. tostring(vx) .. ", vy=" .. tostring(vy))

    -- Temporarily comment out SetEntityAnimation calls to prevent potential crash
    -- if HasEntityComponent(entity, "AnimationComponent") then -- Ideal check
    --    if vx ~= 0 or vy ~= 0 then
    --        SetEntityAnimation(entity, "walk")
    --    else
    --        SetEntityAnimation(entity, "idle")
    --    end
    -- end
end

