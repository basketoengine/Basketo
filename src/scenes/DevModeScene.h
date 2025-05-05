#pragma once

#include "../Scene.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_rect.h> // Ensure SDL_Rect is included
#include "imgui.h"
#include <memory>
#include <string>
#include <fstream>
#include <cmath>
#include <vector>
#include <utility> // For std::pair

#include "../ecs/EntityManager.h"
#include "../ecs/ComponentManager.h"
#include "../ecs/SystemManager.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/systems/RenderSystem.h"
#include "../AssetManager.h"
#include "../ecs/Entity.h"
#include "../../vendor/nlohmann/json.hpp"

const Entity NO_ENTITY_SELECTED = MAX_ENTITIES;
const int HANDLE_SIZE = 8; // Size of the resize handles

// Enum to identify which handle is being interacted with
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

    void handleInput(SDL_Event& event) override; // Modify to accept event
    void update(float deltaTime) override;
    void render() override;

private:
    SDL_Renderer* renderer;
    SDL_Window* window;
    SDL_Rect gameViewport; // Add member to store game viewport dimensions

    // --- Camera/View Offset ---
    float cameraX = 0.0f;
    float cameraY = 0.0f;

    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<ComponentManager> componentManager;
    std::unique_ptr<SystemManager> systemManager;
    std::shared_ptr<RenderSystem> renderSystem;

    ImVec4 clear_color = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Match background color

    float spawnPosX = 100.0f;
    float spawnPosY = 100.0f;
    float spawnSizeW = 32.0f;
    float spawnSizeH = 32.0f;
    char spawnTextureId[256] = ""; // Increased buffer size

    Entity selectedEntity = NO_ENTITY_SELECTED;
    char inspectorTextureIdBuffer[256] = ""; // Increased buffer size

    // --- Save/Load ---
    char sceneFilePath[256] = "scene.json"; // Buffer for filename input

    // --- Viewport Interaction State ---
    bool isDragging = false;
    float dragStartMouseX = 0.0f; // Use float for world coords
    float dragStartMouseY = 0.0f; // Use float for world coords
    float dragStartEntityX = 0.0f;
    float dragStartEntityY = 0.0f;
    float gridSize = 32.0f; // Grid size for snapping
    bool snapToGrid = true; // Toggle for snapping
    bool showGrid = true; // Added showGrid member variable

    // --- Play Mode State ---
    bool isPlaying = false;

    // --- Editor Interaction State ---
    bool isResizing = false;
    ResizeHandle activeHandle = ResizeHandle::NONE;
    float dragStartWidth = 0.0f;
    float dragStartHeight = 0.0f;

    void saveScene(const std::string& filepath);
    void loadScene(const std::string& filepath);

    bool isMouseOverEntity(float worldMouseX, float worldMouseY, Entity entity); // Use float
    // Helper function to get handle rectangles
    std::vector<std::pair<ResizeHandle, SDL_Rect>> getResizeHandles(const TransformComponent& transform);
    // Helper function to check mouse over handles
    ResizeHandle getHandleAtPosition(float worldMouseX, float worldMouseY, const TransformComponent& transform); // Use float
};
