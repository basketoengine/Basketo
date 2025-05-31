local playerSpeed = 150.0 
local jumpForce = 400.0 -- Adjust this value for jump height

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
    -- Log("Player Lua update function called. Entity: " .. tostring(entity) .. ", DeltaTime: " .. tostring(deltaTime)) -- Keep this commented unless deep debugging

    local current_vx, current_vy = 0, 0
    if GetEntityVelocity then
        local vel = GetEntityVelocity(entity)
        if vel then
            current_vx = vel[1]
            current_vy = vel[2]
        else
            -- GetEntityVelocity might return nil if component is missing, error already logged by C++ side
            -- LogError("LUA: GetEntityVelocity(entity) returned nil.") 
        end
    else
        LogError("LUA: GetEntityVelocity function not available. Vertical movement physics might be incorrect.")
    end

    local new_vx = 0
    local new_vy = current_vy

    local a_pressed = Input.isKeyDown("A")
    local d_pressed = Input.isKeyDown("D")

    if a_pressed then
        new_vx = -playerSpeed
    elseif d_pressed then
        new_vx = playerSpeed
    else
        new_vx = 0
    end
    
    local grounded = false
    if IsEntityGrounded then
        grounded = IsEntityGrounded(entity)
    else
        LogError("LUA: IsEntityGrounded function not available. Jump will be disabled.")
    end

    local w_key_for_jump = Input.isKeyDown("W")
    local space_key_for_jump = Input.isKeyDown("Space")
    local jump_key_pressed = w_key_for_jump or space_key_for_jump

    -- More detailed log for jump diagnosis
    Log(string.format("Player Update: W_pressed=%s, Space_pressed=%s, JumpKeyActive=%s, Grounded=%s, current_vy=%.2f", 
        tostring(w_key_for_jump), tostring(space_key_for_jump), tostring(jump_key_pressed), tostring(grounded), current_vy or 0))

    if jump_key_pressed and grounded then
        new_vy = -jumpForce
        Log("Player Jumped! Set new_vy to: " .. tostring(new_vy))
    end

    SetEntityVelocity(entity, new_vx, new_vy)
    -- Log(string.format("Lua set velocity: vx=%.2f, vy=%.2f", new_vx, new_vy)) -- Keep commented unless deep debugging

    -- Temporarily comment out SetEntityAnimation calls to prevent potential crash
    -- if HasEntityComponent(entity, "AnimationComponent") then -- Ideal check
    --    if vx ~= 0 or vy ~= 0 then
    --        SetEntityAnimation(entity, "walk")
    --    else
    --        SetEntityAnimation(entity, "idle")
    --    end
    -- end
end

