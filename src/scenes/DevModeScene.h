#pragma once

#include "../Scene.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h> 
#include "imgui.h"
#include <memory>
#include <string>
#include <fstream>
#include <cmath>
#include <vector>
#include <utility>

#include "../ecs/EntityManager.h"
#include "../ecs/ComponentManager.h"
#include "../ecs/SystemManager.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/ScriptComponent.h" 
#include "../ecs/systems/RenderSystem.h"
#include "../ecs/systems/ScriptSystem.h"
#include "../ecs/systems/MovementSystem.h"
#include "../AssetManager.h"
#include "../ecs/Entity.h"
#include "../../vendor/nlohmann/json.hpp"

class EntityManager;
class ComponentManager;
class SystemManager; 
class RenderSystem; 
class ScriptSystem;

const Entity NO_ENTITY_SELECTED = MAX_ENTITIES;
const int HANDLE_SIZE = 8; 

enum class ResizeHandle {
    NONE,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    // Potentially add side handles later: TOP, BOTTOM, LEFT, RIGHT
};

class DevModeScene : public Scene {
public:
    DevModeScene(SDL_Renderer* ren, SDL_Window* win);
    ~DevModeScene() override;

    void handleInput(SDL_Event& event) override; 
    void update(float deltaTime) override;
    void render() override;

private:
    SDL_Renderer* renderer;
    SDL_Window* window;
    SDL_Rect gameViewport; 

    // --- Camera/View Offset ---
    float cameraX = 0.0f;
    float cameraY = 0.0f;
    float cameraZoom = 1.0f;
    float cameraTargetX = 0.0f, cameraTargetY = 0.0f;

    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<ComponentManager> componentManager; 
    std::shared_ptr<ScriptSystem> scriptSystem; 
    std::unique_ptr<SystemManager> systemManager;
    std::shared_ptr<RenderSystem> renderSystem;
    std::shared_ptr<MovementSystem> movementSystem;
    AssetManager& assetManager;

    // --- UI State & Colors ---
    float hierarchyWidthRatio = 0.18f;
    float inspectorWidthRatio = 0.22f;
    float bottomPanelHeightRatio = 0.25f;
    float topToolbarHeight = 40.0f;
    ImVec4 clear_color = ImVec4(0.27f, 0.51f, 0.71f, 1.00f);

    float spawnPosX = 100.0f;
    float spawnPosY = 100.0f;
    float spawnSizeW = 32.0f;
    float spawnSizeH = 32.0f;
    char spawnTextureId[256] = "";

    Entity selectedEntity = NO_ENTITY_SELECTED;
    char inspectorTextureIdBuffer[256] = "";
    char inspectorScriptPathBuffer[256] = ""; 

    // --- Save/Load ---
    char sceneFilePath[256] = "scene.json";

    // --- Viewport Interaction State ---
    bool isDragging = false;
    float dragStartMouseX = 0.0f; 
    float dragStartMouseY = 0.0f; 
    float dragStartEntityX = 0.0f;
    float dragStartEntityY = 0.0f;
    float gridSize = 32.0f; 
    bool snapToGrid = true; 
    bool showGrid = true; 

    // --- Play Mode State ---
    bool isPlaying = false;

    // --- Editor Interaction State ---
    bool isResizing = false;
    ResizeHandle activeHandle = ResizeHandle::NONE;
    float dragStartWidth = 0.0f;
    float dragStartHeight = 0.0f;

    void saveScene(const std::string& filepath);
    void loadScene(const std::string& filepath);

    bool isMouseOverEntity(float worldMouseX, float worldMouseY, Entity entity); 
    
    std::vector<std::pair<ResizeHandle, SDL_Rect>> getResizeHandles(const TransformComponent& transform);
    
    ResizeHandle getHandleAtPosition(float worldMouseX, float worldMouseY, const TransformComponent& transform); // Use float
};
