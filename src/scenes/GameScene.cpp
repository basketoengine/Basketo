#include "GameScene.h"
#include "../InputManager.h"
#include "../AssetManager.h" // Include AssetManager
#include "../../utils/utility.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/VelocityComponent.h"
#include "../ecs/components/SpriteComponent.h" // Include SpriteComponent
#include <SDL2/SDL_image.h>
#include <iostream>
#include <memory>

GameScene::GameScene(SDL_Renderer* ren) : renderer(ren) {
    // Initialize ECS managers
    entityManager = std::make_unique<EntityManager>();
    componentManager = std::make_unique<ComponentManager>();
    systemManager = std::make_unique<SystemManager>(); 

    // Register components
    componentManager->registerComponent<TransformComponent>();
    componentManager->registerComponent<VelocityComponent>();
    componentManager->registerComponent<SpriteComponent>(); // Register SpriteComponent

    // Register systems and set signatures
    renderSystem = systemManager->registerSystem<RenderSystem>();
    Signature renderSig;
    renderSig.set(componentManager->getComponentType<TransformComponent>());
    renderSig.set(componentManager->getComponentType<SpriteComponent>()); // Add SpriteComponent to signature
    systemManager->setSignature<RenderSystem>(renderSig);

    movementSystem = systemManager->registerSystem<MovementSystem>(); 
    Signature moveSig;
    moveSig.set(componentManager->getComponentType<TransformComponent>());
    moveSig.set(componentManager->getComponentType<VelocityComponent>());
    systemManager->setSignature<MovementSystem>(moveSig);

    // Load assets using AssetManager
    if (!AssetManager::getInstance().loadTexture("logo", "../assets/Image/logo.png")) {
        std::cerr << "GameScene Error: Failed to load logo texture!" << std::endl;
    }
    if (!AssetManager::getInstance().loadTexture("player", "../assets/Image/player.png")) {
        std::cerr << "GameScene Error: Failed to load player texture!" << std::endl;
    }

    playerEntity = entityManager->createEntity();

    // Player components
    TransformComponent playerTransform;
    playerTransform.x = 100.0f;
    playerTransform.y = 100.0f;
    playerTransform.width = 64.0f; // Adjust size based on player.png
    playerTransform.height = 64.0f; 
    componentManager->addComponent(playerEntity, playerTransform); 
    componentManager->addComponent(playerEntity, VelocityComponent{0.0f, 0.0f});
    componentManager->addComponent(playerEntity, SpriteComponent{"player"}); // Add SpriteComponent

    // Update player signature
    Signature playerSignature = entityManager->getSignature(playerEntity);
    playerSignature.set(componentManager->getComponentType<TransformComponent>());
    playerSignature.set(componentManager->getComponentType<VelocityComponent>());
    playerSignature.set(componentManager->getComponentType<SpriteComponent>()); // Add SpriteComponent to signature
    entityManager->setSignature(playerEntity, playerSignature);
    systemManager->entitySignatureChanged(playerEntity, playerSignature); // Notify systems

    // Create wall entity
    wallEntity = entityManager->createEntity();
    TransformComponent wallTransform;
    wallTransform.x = 300.0f;
    wallTransform.y = 100.0f;
    wallTransform.width = 100.0f; 
    wallTransform.height = 100.0f; 
    componentManager->addComponent(wallEntity, wallTransform);
    // Use logo texture for the wall for now
    componentManager->addComponent(wallEntity, SpriteComponent{"logo"}); // Add SpriteComponent

    // Update wall signature
    Signature wallSignature = entityManager->getSignature(wallEntity);
    wallSignature.set(componentManager->getComponentType<TransformComponent>());
    wallSignature.set(componentManager->getComponentType<SpriteComponent>()); // Add SpriteComponent to signature
    entityManager->setSignature(wallEntity, wallSignature);
    systemManager->entitySignatureChanged(wallEntity, wallSignature); // Notify systems

    InputManager::getInstance().mapAction("MoveLeft", SDL_SCANCODE_A);
    InputManager::getInstance().mapAction("MoveRight", SDL_SCANCODE_D);
    InputManager::getInstance().mapAction("MoveUp", SDL_SCANCODE_W);
    InputManager::getInstance().mapAction("MoveDown", SDL_SCANCODE_S);
}

GameScene::~GameScene() {
}

void GameScene::handleInput() {
    // InputManager::getInstance().update(); // Removed redundant update call
}

void GameScene::update(float deltaTime) {
    float speed = 200.0f;
    auto& playerVelocity = componentManager->getComponent<VelocityComponent>(playerEntity);
    playerVelocity.vx = 0.0f;
    playerVelocity.vy = 0.0f;

    if (InputManager::getInstance().isActionPressed("MoveLeft"))  playerVelocity.vx -= speed;
    if (InputManager::getInstance().isActionPressed("MoveRight")) playerVelocity.vx += speed;
    if (InputManager::getInstance().isActionPressed("MoveUp"))    playerVelocity.vy -= speed;
    if (InputManager::getInstance().isActionPressed("MoveDown"))  playerVelocity.vy += speed;

    movementSystem->update(componentManager.get(), deltaTime);

    // Collision detection and response (needs to be moved to a CollisionSystem)
    auto& playerTransform = componentManager->getComponent<TransformComponent>(playerEntity);
    auto& wallTransform = componentManager->getComponent<TransformComponent>(wallEntity);
    SDL_Rect playerRect = { (int)playerTransform.x, (int)playerTransform.y, (int)playerTransform.width, (int)playerTransform.height };
    SDL_Rect wallRect = { (int)wallTransform.x, (int)wallTransform.y, (int)wallTransform.width, (int)wallTransform.height };

    // Simple collision check - if collision, revert player position (crude)
    if (SDL_HasIntersection(&playerRect, &wallRect)) {
        playerTransform.x -= playerVelocity.vx * deltaTime;
        playerTransform.y -= playerVelocity.vy * deltaTime;
        playerVelocity.vx = 0;
        playerVelocity.vy = 0;
    }

    // Boundary checks (could be part of MovementSystem or a dedicated BoundarySystem)
    if (playerTransform.x < 0) playerTransform.x = 0;
    if (playerTransform.y < 0) playerTransform.y = 0;
    if (playerTransform.x + playerTransform.width > 800) playerTransform.x = 800 - playerTransform.width;
    if (playerTransform.y + playerTransform.height > 600) playerTransform.y = 600 - playerTransform.height;
}

void GameScene::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    // Render entities via RenderSystem
    renderSystem->update(renderer, componentManager.get());

    SDL_RenderPresent(renderer);
}
