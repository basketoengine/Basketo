-- Event System & State Machine Demo Script
-- This script demonstrates the advanced event system and state machine framework

local demoTimer = 0.0
local currentDemo = 1
local demoDuration = 5.0  -- Switch demos every 5 seconds
local demos = {"player", "enemy", "events"}

function init(entity)
    Log("Event & State Machine Demo script initialized for entity: " .. tostring(entity))
    
    -- Start with player controller demo
    CreatePlayerStateMachine(entity)
    Log("Created player state machine")
    
    -- Add event component for event handling
    AddEventListener(entity, "demo_event")
    Log("Added event listener for demo_event")
end

function update(entity, deltaTime)
    demoTimer = demoTimer + deltaTime
    
    -- Switch between different demos
    if demoTimer >= demoDuration then
        demoTimer = 0.0
        currentDemo = currentDemo + 1
        if currentDemo > #demos then
            currentDemo = 1
        end
        
        local demoName = demos[currentDemo]
        Log("Switching to " .. demoName .. " demo")
        
        -- Switch demo based on current selection
        if demoName == "player" then
            -- Player controller state machine demo
            CreatePlayerStateMachine(entity)
            Log("Created player state machine")
            
            -- Simulate player input events
            if math.random() > 0.5 then
                ChangeState(entity, "Walking")
                Log("Player started walking")
            else
                ChangeState(entity, "Jumping")
                Log("Player started jumping")
            end
            
        elseif demoName == "enemy" then
            -- Enemy AI state machine demo
            CreateEnemyStateMachine(entity)
            Log("Created enemy AI state machine")
            
            -- Simulate AI events
            if math.random() > 0.7 then
                ChangeState(entity, "Chase")
                Log("Enemy started chasing")
            elseif math.random() > 0.5 then
                ChangeState(entity, "Attack")
                Log("Enemy started attacking")
            else
                ChangeState(entity, "Patrol")
                Log("Enemy started patrolling")
            end
            
        elseif demoName == "events" then
            -- Event system demo
            Log("Demonstrating event system")
            
            -- Send various events
            SendEvent(entity, "demo_event")
            SendEvent(entity, "collision_detected")
            SendEvent(entity, "health_changed")
            SendEvent(entity, "score_updated")
            
            Log("Sent multiple demo events")
        end
    end
    
    -- Log current state every 2 seconds
    if math.floor(demoTimer * 10) % 20 == 0 then
        local currentState = GetCurrentState(entity)
        if currentState ~= "" then
            Log("Current state: " .. currentState)
        end
    end
    
    -- Handle keyboard input for manual control
    if Input.isKeyDown("q") then
        -- Player states
        ChangeState(entity, "Idle")
        Log("Manual state change: Idle")
    elseif Input.isKeyDown("w") then
        ChangeState(entity, "Walking")
        Log("Manual state change: Walking")
    elseif Input.isKeyDown("e") then
        ChangeState(entity, "Jumping")
        Log("Manual state change: Jumping")
    elseif Input.isKeyDown("r") then
        ChangeState(entity, "Attacking")
        Log("Manual state change: Attacking")
    elseif Input.isKeyDown("t") then
        -- Enemy states
        ChangeState(entity, "Patrol")
        Log("Manual state change: Patrol")
    elseif Input.isKeyDown("y") then
        ChangeState(entity, "Chase")
        Log("Manual state change: Chase")
    elseif Input.isKeyDown("u") then
        ChangeState(entity, "Attack")
        Log("Manual state change: Attack")
    elseif Input.isKeyDown("i") then
        ChangeState(entity, "Flee")
        Log("Manual state change: Flee")
    elseif Input.isKeyDown("o") then
        -- Send custom events
        SendEvent(entity, "custom_event_1")
        Log("Sent custom_event_1")
    elseif Input.isKeyDown("p") then
        SendEvent(entity, "custom_event_2")
        Log("Sent custom_event_2")
    end
    
    -- State-specific behavior
    local currentState = GetCurrentState(entity)
    
    if currentState == "Walking" then
        -- Simulate walking behavior
        if math.random() > 0.95 then -- 5% chance per frame
            SendEvent(entity, "footstep_sound")
        end
    elseif currentState == "Attacking" then
        -- Simulate attack behavior
        if math.random() > 0.9 then -- 10% chance per frame
            SendEvent(entity, "attack_hit")
        end
    elseif currentState == "Chase" then
        -- Simulate chase behavior
        if math.random() > 0.98 then -- 2% chance per frame
            SendEvent(entity, "player_in_range")
        end
    end
    
    -- Check specific states
    if IsInState(entity, "Jumping") then
        -- Special jumping logic
        if math.random() > 0.99 then -- 1% chance per frame
            SendEvent(entity, "landing_sound")
        end
    end
end

-- Event callback function (would be called by event system)
function onEvent(entity, eventName, eventData)
    Log("Event received: " .. eventName .. " on entity " .. tostring(entity))
    
    -- Handle different event types
    if eventName == "demo_event" then
        Log("Demo event processed!")
    elseif eventName == "collision_detected" then
        Log("Collision detected - changing to defensive state")
        if GetCurrentState(entity) == "Patrol" then
            ChangeState(entity, "Flee")
        end
    elseif eventName == "health_changed" then
        Log("Health changed - checking if low health")
        -- Could trigger state change based on health
    elseif eventName == "attack_hit" then
        Log("Attack hit - dealing damage")
        SendEvent(entity, "damage_dealt")
    end
end
