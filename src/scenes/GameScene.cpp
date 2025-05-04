#include "GameScene.h"
#include "../InputManager.h"
#include "../AssetManager.h"
#include "../../utils/utility.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/VelocityComponent.h"
#include "../ecs/components/SpriteComponent.h" 
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>  
#include <SDL2/SDL_mixer.h>
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
    componentManager->registerComponent<SpriteComponent>(); 

    // Register systems and set signatures
    renderSystem = systemManager->registerSystem<RenderSystem>();
    Signature renderSig;
    renderSig.set(componentManager->getComponentType<TransformComponent>());
    renderSig.set(componentManager->getComponentType<SpriteComponent>());
    systemManager->setSignature<RenderSystem>(renderSig);

    movementSystem = systemManager->registerSystem<MovementSystem>(); 
    Signature moveSig;
    moveSig.set(componentManager->getComponentType<TransformComponent>());
    moveSig.set(componentManager->getComponentType<VelocityComponent>());
    systemManager->setSignature<MovementSystem>(moveSig);

    // Load assets using AssetManager
    AssetManager& assets = AssetManager::getInstance();

    if (!assets.loadTexture("logo", "../assets/Image/logo.png")) {
        std::cerr << "GameScene Error: Failed to load logo texture!" << std::endl;
    }
    if (!assets.loadTexture("player", "../assets/Image/player.png")) {
        std::cerr << "GameScene Error: Failed to load player texture!" << std::endl;
    }

    if (!assets.loadFont("roboto_16", "../assets/fonts/roboto/Roboto-Regular.ttf", 16)) {
         std::cerr << "GameScene Error: Failed to load roboto font size 16!" << std::endl;
    }
    if (!assets.loadSound("test_sound", "../assets/Sound/test.mp3")) { 
         std::cerr << "GameScene Error: Failed to load test sound!" << std::endl;
    }

    playerEntity = entityManager->createEntity();

    TransformComponent playerTransform;
    playerTransform.x = 100.0f;
    playerTransform.y = 100.0f;
    playerTransform.width = 64.0f;
    playerTransform.height = 64.0f; 
    componentManager->addComponent(playerEntity, playerTransform); 
    componentManager->addComponent(playerEntity, VelocityComponent{0.0f, 0.0f});
    componentManager->addComponent(playerEntity, SpriteComponent{"player"});

    Signature playerSignature = entityManager->getSignature(playerEntity);
    playerSignature.set(componentManager->getComponentType<TransformComponent>());
    playerSignature.set(componentManager->getComponentType<VelocityComponent>());
    playerSignature.set(componentManager->getComponentType<SpriteComponent>());
    entityManager->setSignature(playerEntity, playerSignature);
    systemManager->entitySignatureChanged(playerEntity, playerSignature); 

    wallEntity = entityManager->createEntity();
    TransformComponent wallTransform;
    wallTransform.x = 300.0f;
    wallTransform.y = 100.0f;
    wallTransform.width = 100.0f; 
    wallTransform.height = 100.0f; 
    componentManager->addComponent(wallEntity, wallTransform);

    componentManager->addComponent(wallEntity, SpriteComponent{"logo"}); 

    Signature wallSignature = entityManager->getSignature(wallEntity);
    wallSignature.set(componentManager->getComponentType<TransformComponent>());
    wallSignature.set(componentManager->getComponentType<SpriteComponent>());
    entityManager->setSignature(wallEntity, wallSignature);
    systemManager->entitySignatureChanged(wallEntity, wallSignature); // Notify systems

    InputManager::getInstance().mapAction("MoveLeft", SDL_SCANCODE_A);
    InputManager::getInstance().mapAction("MoveRight", SDL_SCANCODE_D);
    InputManager::getInstance().mapAction("MoveUp", SDL_SCANCODE_W);
    InputManager::getInstance().mapAction("MoveDown", SDL_SCANCODE_S);

    InputManager::getInstance().mapAction("PlaySound", SDL_SCANCODE_SPACE);
}

GameScene::~GameScene() {
}

void GameScene::handleInput(SDL_Event& event) { // Changed signature
    // Process specific events if needed, or rely on InputManager for polling actions
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
         if (InputManager::getInstance().isActionPressed("PlaySound")) { // Check action mapping
            Mix_Chunk* sound = AssetManager::getInstance().getSound("test_sound");
            if (sound) {
                Mix_PlayChannel(-1, sound, 0);
                std::cout << "Played test_sound via event" << std::endl;
            } else {
                std::cerr << "Could not get test_sound to play." << std::endl;
            }
        }
    }
    // Example: Check for mouse clicks specific to this scene (if not handled by ImGui)
    // if (event.type == SDL_MOUSEBUTTONDOWN) { ... }
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

    // Test font rendering - Use the combined key "roboto_16_16"
    TTF_Font* font = AssetManager::getInstance().getFont("roboto_16_16");
    if (font) {
        SDL_Color textColor = {255, 255, 255, 255}; // White color
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, "AssetManager Test!", textColor);
        if (textSurface) {
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture) {
                SDL_Rect textRect;
                textRect.x = 10; // Position on screen
                textRect.y = 10;
                textRect.w = textSurface->w;
                textRect.h = textSurface->h;
                SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                SDL_DestroyTexture(textTexture); // Clean up texture
            } else {
                 std::cerr << "Failed to create text texture: " << SDL_GetError() << std::endl;
            }
            SDL_FreeSurface(textSurface); // Clean up surface
        } else {
             std::cerr << "Failed to render text surface: " << TTF_GetError() << std::endl;
        }
    } else {
         // Updated error message to reflect the key used
         std::cerr << "Could not get font roboto_16_16 for rendering." << std::endl;
    }

    SDL_RenderPresent(renderer);
}
