#pragma once

#include "../Scene.h"
#include <SDL2/SDL.h>
#include "imgui.h"
#include <memory> 
#include <string> 

#include "../ecs/EntityManager.h"
#include "../ecs/ComponentManager.h"
#include "../ecs/SystemManager.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/systems/RenderSystem.h"
#include "../AssetManager.h" 
#include "../ecs/Entity.h"

const Entity NO_ENTITY_SELECTED = MAX_ENTITIES;

class DevModeScene : public Scene {
public:
    DevModeScene(SDL_Renderer* ren, SDL_Window* win);
    ~DevModeScene() override;

    void handleInput() override;
    void update(float deltaTime) override;
    void render() override;

private:
    SDL_Renderer* renderer;
    SDL_Window* window; 

    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<ComponentManager> componentManager;
    std::unique_ptr<SystemManager> systemManager;
    std::shared_ptr<RenderSystem> renderSystem;

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    float spawnPosX = 100.0f;
    float spawnPosY = 100.0f;
    float spawnSizeW = 32.0f;
    float spawnSizeH = 32.0f;
    bool spawnAddSprite = false;
    char spawnTextureId[64] = "logo"; 

    Entity selectedEntity = NO_ENTITY_SELECTED; 
    char inspectorTextureIdBuffer[64] = ""; 

};
