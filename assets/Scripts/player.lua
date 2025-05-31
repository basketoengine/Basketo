local playerSpeed = 150.0 
local jumpForce = 400.0 

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
    local current_vx, current_vy = 0, 0
    if GetEntityVelocity then
        local vel = GetEntityVelocity(entity)
        if vel then
            current_vx = vel[1]
            current_vy = vel[2]
        else
           
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
    local contacts = GetCollisionContacts(entity)
    if contacts then
        for i, contact in ipairs(contacts) do
           
            if contact.normalY and contact.normalY < -0.5 then 
                grounded = true
                new_vy = 0
                break
            end
        end
    end

    local w_key_for_jump = Input.isKeyDown("W")
    local space_key_for_jump = Input.isKeyDown("Space")
    local jump_key_pressed = w_key_for_jump or space_key_for_jump

    if jump_key_pressed and grounded then
        new_vy = -jumpForce
        Log("Player Jumped! Set new_vy to: " .. tostring(new_vy))
    end
    Log("is grounded: " .. tostring(grounded))
  
    SetEntityVelocity(entity, new_vx, new_vy)
    Log("HasEntityComponent: " .. tostring(HasEntityComponent(entity, "AnimationComponent")))
    if HasEntityComponent(entity, "AnimationComponent") then
        local current_anim_name = GetEntityAnimation(entity) 
        local target_anim_name = "idle" 
        Log("LUA: Current animation name: " .. tostring(current_anim_name))
        if grounded then
            Log("LUA: Player is grounded.")
            if new_vx ~= 0 then
                Log("LUA: Player is moving on the ground.")
                target_anim_name = "walk"
            end
           
        else 
           
            if new_vx ~= 0 then
                target_anim_name = "walk"
            else
                target_anim_name = "idle"
            end
        end

        Log(string.format("LUA: Player anim state. Current: '%s', Target: '%s', Grounded: %s, vx: %.2f",
            current_anim_name or "nil",
            target_anim_name,
            tostring(grounded),
            new_vx))

        if current_anim_name ~= target_anim_name and target_anim_name ~= "" then
            Log(string.format("LUA: Attempting to set animation from '%s' to '%s'", current_anim_name or "nil", target_anim_name))
            SetEntityAnimation(entity, target_anim_name)
        end
    end
end

