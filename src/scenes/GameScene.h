#pragma once
#include "../Scene.h"
#include "../ecs/EntityManager.h"
#include "../ecs/ComponentManager.h"
#include "../ecs/SystemManager.h" 
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/VelocityComponent.h"
#include "../ecs/systems/RenderSystem.h" 
#include "../ecs/systems/MovementSystem.h" 
#include "../ecs/Types.h"
#include <SDL2/SDL.h>
#include <string>
#include <memory> 

class GameScene : public Scene {
public:
    GameScene(SDL_Renderer* renderer);
    ~GameScene() override;
    void handleInput(SDL_Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    SDL_Renderer* renderer;
    Entity playerEntity;
    Entity wallEntity; 
    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<ComponentManager> componentManager;
    std::unique_ptr<SystemManager> systemManager; 
    std::shared_ptr<MovementSystem> movementSystem; 
    std::shared_ptr<RenderSystem> renderSystem;
    SDL_Texture* loadTexture(const std::string& path);
};
