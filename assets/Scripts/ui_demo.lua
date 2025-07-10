-- Advanced UI System Demo Script
-- This script demonstrates the comprehensive UI system with interactive elements

local demoTimer = 0.0
local currentDemo = 1
local demoDuration = 4.0  -- Switch demos every 4 seconds
local demos = {"buttons", "text", "panels", "sliders"}
local uiElements = {}  -- Track created UI elements

function init(entity)
    Log("Advanced UI System Demo script initialized for entity: " .. tostring(entity))
    
    -- Start with button demo
    createButtonDemo(entity)
    Log("Created button demo UI")
end

function update(entity, deltaTime)
    demoTimer = demoTimer + deltaTime
    
    -- Switch between different UI demos
    if demoTimer >= demoDuration then
        demoTimer = 0.0
        
        -- Clear previous UI elements
        clearUIElements()
        
        currentDemo = currentDemo + 1
        if currentDemo > #demos then
            currentDemo = 1
        end
        
        local demoName = demos[currentDemo]
        Log("Switching to " .. demoName .. " demo")
        
        -- Create appropriate UI demo
        if demoName == "buttons" then
            createButtonDemo(entity)
        elseif demoName == "text" then
            createTextDemo(entity)
        elseif demoName == "panels" then
            createPanelDemo(entity)
        elseif demoName == "sliders" then
            createSliderDemo(entity)
        end
    end
    
    -- Update UI element animations and interactions
    updateUIAnimations(entity, deltaTime)
    
    -- Handle keyboard shortcuts for manual demo switching
    if Input.isKeyDown("1") then
        clearUIElements()
        createButtonDemo(entity)
        Log("Manual switch to button demo")
        demoTimer = 0.0
    elseif Input.isKeyDown("2") then
        clearUIElements()
        createTextDemo(entity)
        Log("Manual switch to text demo")
        demoTimer = 0.0
    elseif Input.isKeyDown("3") then
        clearUIElements()
        createPanelDemo(entity)
        Log("Manual switch to panel demo")
        demoTimer = 0.0
    elseif Input.isKeyDown("4") then
        clearUIElements()
        createSliderDemo(entity)
        Log("Manual switch to slider demo")
        demoTimer = 0.0
    elseif Input.isKeyDown("5") then
        -- Toggle visibility of all UI elements
        toggleUIVisibility()
        Log("Toggled UI visibility")
    end
end

function createButtonDemo(entity)
    Log("Creating interactive button demo")
    
    -- Create main panel
    local panelEntity = CreateEntity()
    CreatePanel(panelEntity, 50, 50, 300, 200)
    table.insert(uiElements, panelEntity)
    
    -- Create title text
    local titleEntity = CreateEntity()
    CreateText(titleEntity, "Button Demo", 60, 60)
    table.insert(uiElements, titleEntity)
    
    -- Create interactive buttons
    local button1 = CreateEntity()
    CreateButton(button1, "Click Me!", 60, 100, 120, 40)
    table.insert(uiElements, button1)
    
    local button2 = CreateEntity()
    CreateButton(button2, "Press Here", 200, 100, 120, 40)
    table.insert(uiElements, button2)
    
    local button3 = CreateEntity()
    CreateButton(button3, "Action Button", 60, 160, 120, 40)
    table.insert(uiElements, button3)
    
    local button4 = CreateEntity()
    CreateButton(button4, "Demo Button", 200, 160, 120, 40)
    table.insert(uiElements, button4)
    
    Log("Created " .. #uiElements .. " UI elements for button demo")
end

function createTextDemo(entity)
    Log("Creating dynamic text demo")
    
    -- Create main panel
    local panelEntity = CreateEntity()
    CreatePanel(panelEntity, 400, 50, 350, 250)
    table.insert(uiElements, panelEntity)
    
    -- Create various text elements
    local titleEntity = CreateEntity()
    CreateText(titleEntity, "Dynamic Text Demo", 410, 60)
    table.insert(uiElements, titleEntity)
    
    local infoEntity = CreateEntity()
    CreateText(infoEntity, "This text updates dynamically", 410, 100)
    table.insert(uiElements, infoEntity)
    
    local counterEntity = CreateEntity()
    CreateText(counterEntity, "Counter: 0", 410, 140)
    table.insert(uiElements, counterEntity)
    
    local statusEntity = CreateEntity()
    CreateText(statusEntity, "Status: Active", 410, 180)
    table.insert(uiElements, statusEntity)
    
    local timeEntity = CreateEntity()
    CreateText(timeEntity, "Time: 0.0s", 410, 220)
    table.insert(uiElements, timeEntity)
    
    Log("Created text demo with dynamic content")
end

function createPanelDemo(entity)
    Log("Creating nested panel demo")
    
    -- Create main container panel
    local mainPanel = CreateEntity()
    CreatePanel(mainPanel, 100, 300, 400, 300)
    table.insert(uiElements, mainPanel)
    
    -- Create title
    local titleEntity = CreateEntity()
    CreateText(titleEntity, "Nested Panel Demo", 110, 310)
    table.insert(uiElements, titleEntity)
    
    -- Create nested panels
    local panel1 = CreateEntity()
    CreatePanel(panel1, 120, 350, 150, 100)
    table.insert(uiElements, panel1)
    
    local panel1Text = CreateEntity()
    CreateText(panel1Text, "Panel 1", 130, 360)
    table.insert(uiElements, panel1Text)
    
    local panel2 = CreateEntity()
    CreatePanel(panel2, 290, 350, 150, 100)
    table.insert(uiElements, panel2)
    
    local panel2Text = CreateEntity()
    CreateText(panel2Text, "Panel 2", 300, 360)
    table.insert(uiElements, panel2Text)
    
    -- Create sub-panels
    local subPanel1 = CreateEntity()
    CreatePanel(subPanel1, 120, 470, 100, 80)
    table.insert(uiElements, subPanel1)
    
    local subPanel2 = CreateEntity()
    CreatePanel(subPanel2, 240, 470, 100, 80)
    table.insert(uiElements, subPanel2)
    
    local subPanel3 = CreateEntity()
    CreatePanel(subPanel3, 360, 470, 100, 80)
    table.insert(uiElements, subPanel3)
    
    Log("Created nested panel layout")
end

function createSliderDemo(entity)
    Log("Creating interactive slider demo")
    
    -- Create main panel
    local panelEntity = CreateEntity()
    CreatePanel(panelEntity, 550, 300, 300, 250)
    table.insert(uiElements, panelEntity)
    
    -- Create title
    local titleEntity = CreateEntity()
    CreateText(titleEntity, "Slider Controls", 560, 310)
    table.insert(uiElements, titleEntity)
    
    -- Create sliders with labels
    local volumeLabel = CreateEntity()
    CreateText(volumeLabel, "Volume: 50%", 560, 350)
    table.insert(uiElements, volumeLabel)
    
    local volumeSlider = CreateEntity()
    CreateSlider(volumeSlider, 0, 100, 50, 560, 370, 200, 20)
    table.insert(uiElements, volumeSlider)
    
    local brightnessLabel = CreateEntity()
    CreateText(brightnessLabel, "Brightness: 75%", 560, 410)
    table.insert(uiElements, brightnessLabel)
    
    local brightnessSlider = CreateEntity()
    CreateSlider(brightnessSlider, 0, 100, 75, 560, 430, 200, 20)
    table.insert(uiElements, brightnessSlider)
    
    local speedLabel = CreateEntity()
    CreateText(speedLabel, "Speed: 25%", 560, 470)
    table.insert(uiElements, speedLabel)
    
    local speedSlider = CreateEntity()
    CreateSlider(speedSlider, 0, 100, 25, 560, 490, 200, 20)
    table.insert(uiElements, speedSlider)
    
    Log("Created interactive slider controls")
end

function updateUIAnimations(entity, deltaTime)
    -- Update dynamic text content
    if demos[currentDemo] == "text" and #uiElements >= 5 then
        -- Update counter
        local counter = math.floor(demoTimer * 10) % 100
        SetUIText(uiElements[3], "Counter: " .. counter)
        
        -- Update status
        local status = (math.floor(demoTimer) % 2 == 0) and "Active" or "Standby"
        SetUIText(uiElements[4], "Status: " .. status)
        
        -- Update time
        SetUIText(uiElements[5], "Time: " .. string.format("%.1f", demoTimer) .. "s")
    end
    
    -- Animate panel positions
    if demos[currentDemo] == "panels" and #uiElements > 0 then
        local offset = math.sin(demoTimer * 2) * 10
        for i, element in ipairs(uiElements) do
            if i > 2 then -- Skip main panel and title
                local baseX = 120 + ((i - 3) % 3) * 120
                SetUIPosition(element, baseX + offset, 350 + math.floor((i - 3) / 3) * 120)
            end
        end
    end
end

function clearUIElements()
    -- Hide all UI elements (in a real implementation, we'd destroy them)
    for _, element in ipairs(uiElements) do
        SetUIVisible(element, false)
    end
    uiElements = {}
    Log("Cleared UI elements")
end

function toggleUIVisibility()
    local visible = (math.floor(demoTimer) % 2 == 0)
    for _, element in ipairs(uiElements) do
        SetUIVisible(element, visible)
    end
end

-- Event handlers for UI interactions
function onButtonClicked(buttonEntity)
    Log("Button clicked: " .. tostring(buttonEntity))
    SendEvent(buttonEntity, "button_clicked")
end

function onSliderChanged(sliderEntity, value)
    Log("Slider changed: " .. tostring(sliderEntity) .. " = " .. tostring(value))
    SendEvent(sliderEntity, "slider_changed")
end

function onTextChanged(inputEntity, text)
    Log("Text changed: " .. tostring(inputEntity) .. " = '" .. text .. "'")
    SendEvent(inputEntity, "text_changed")
end
