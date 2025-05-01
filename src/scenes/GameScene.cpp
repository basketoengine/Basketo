#include "GameScene.h"
#include "../InputManager.h"
#include "../../utils/utility.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/VelocityComponent.h"
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

    // Register systems and set signatures
    renderSystem = systemManager->registerSystem<RenderSystem>();
    Signature renderSig;
    renderSig.set(componentManager->getComponentType<TransformComponent>());
    systemManager->setSignature<RenderSystem>(renderSig);

    movementSystem = systemManager->registerSystem<MovementSystem>(); 
    Signature moveSig;
    moveSig.set(componentManager->getComponentType<TransformComponent>());
    moveSig.set(componentManager->getComponentType<VelocityComponent>());
    systemManager->setSignature<MovementSystem>(moveSig);

    playerEntity = entityManager->createEntity();

    // I need to remove this shit
    TransformComponent playerTransform;
    playerTransform.x = 100.0f;
    playerTransform.y = 100.0f;
    playerTransform.width = 50.0f; // Explicitly set width not cool here
    playerTransform.height = 50.0f; 

    componentManager->addComponent(playerEntity, playerTransform); 
    componentManager->addComponent(playerEntity, VelocityComponent{0.0f, 0.0f});
    // Update player signature
    Signature playerSignature = entityManager->getSignature(playerEntity);
    playerSignature.set(componentManager->getComponentType<TransformComponent>());
    playerSignature.set(componentManager->getComponentType<VelocityComponent>());
    entityManager->setSignature(playerEntity, playerSignature);
    systemManager->entitySignatureChanged(playerEntity, playerSignature); // Notify systems

    // Create wall entity
    wallEntity = entityManager->createEntity();
    // need to be removed too
    TransformComponent wallTransform;
    wallTransform.x = 300.0f;
    wallTransform.y = 100.0f;
    wallTransform.width = 100.0f; 
    wallTransform.height = 100.0f; 
    
    componentManager->addComponent(wallEntity, wallTransform);

    Signature wallSignature = entityManager->getSignature(wallEntity);
    wallSignature.set(componentManager->getComponentType<TransformComponent>());
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

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect = { 100, 100, 200, 150 };
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    drawCircle(renderer, 400, 300, 50);

    SDL_Texture* texture = loadTexture("../assets/Image/logo.png");
    if (texture) {
        SDL_Rect destRect = { 300, 200, 100, 64 };
        SDL_RenderCopy(renderer, texture, nullptr, &destRect);
        SDL_DestroyTexture(texture);
    }

    renderSystem->update(renderer, componentManager.get());

    SDL_RenderPresent(renderer);
}

SDL_Texture* GameScene::loadTexture(const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "IMG_Load Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    SDL_SetTextureColorMod(texture, 255, 255, 255);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}
