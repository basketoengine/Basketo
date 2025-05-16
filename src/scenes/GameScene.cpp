#include "GameScene.h"
#include "../InputManager.h"
#include "../AssetManager.h"
#include "../../utils/utility.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/VelocityComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/RigidbodyComponent.h"
#include "../ecs/components/AnimationComponent.h"
#include "../ecs/systems/PhysicsSystem.h"
#include "../ecs/systems/CollisionSystem.h"
#include "../ecs/systems/AnimationSystem.h" 
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <memory>
#include <algorithm>
#include <filesystem>

GameScene::GameScene(SDL_Renderer* ren) : renderer(ren), cameraZoom(1.0f), cameraX(0.0f), cameraY(0.0f) {
    entityManager = std::make_unique<EntityManager>();
    componentManager = std::make_unique<ComponentManager>();
    systemManager = std::make_unique<SystemManager>();

    componentManager->registerComponent<TransformComponent>();
    componentManager->registerComponent<SpriteComponent>();
    componentManager->registerComponent<VelocityComponent>();
    componentManager->registerComponent<RigidbodyComponent>(); // Register RigidbodyComponent
    componentManager->registerComponent<AnimationComponent>(); // Register AnimationComponent

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

    physicsSystem = systemManager->registerSystem<PhysicsSystem>();
    Signature physicsSig;
    physicsSig.set(componentManager->getComponentType<VelocityComponent>());
    physicsSig.set(componentManager->getComponentType<RigidbodyComponent>());
    systemManager->setSignature<PhysicsSystem>(physicsSig);

    collisionSystem = systemManager->registerSystem<CollisionSystem>();
    Signature collisionSig;
    collisionSig.set(componentManager->getComponentType<TransformComponent>());
    collisionSig.set(componentManager->getComponentType<RigidbodyComponent>());
    systemManager->setSignature<CollisionSystem>(collisionSig);

    animationSystem = systemManager->registerSystem<AnimationSystem>(); // Register AnimationSystem
    Signature animSig;
    animSig.set(componentManager->getComponentType<SpriteComponent>());
    animSig.set(componentManager->getComponentType<AnimationComponent>());
    systemManager->setSignature<AnimationSystem>(animSig);

    AssetManager& assets = AssetManager::getInstance();

    std::string texturePath = "../assets/Textures/";
    if (std::filesystem::exists(texturePath)) {
        for (const auto& entry : std::filesystem::directory_iterator(texturePath)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string id = entry.path().stem().string();
                if (!assets.loadTexture(id, path)) {
                    std::cerr << "GameScene Error: Failed to load texture: " << path << std::endl;
                }
            }
        }
    }

    std::string audioPath = "../assets/Audio/";
    if (std::filesystem::exists(audioPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(audioPath)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string id = entry.path().stem().string();
                if (!assets.loadSound(id, path)) {
                    std::cerr << "GameScene Error: Failed to load sound: " << path << std::endl;
                }
            }
        }
    }

    std::string fontBasePath = "../assets/Fonts/";
    if (std::filesystem::exists(fontBasePath)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(fontBasePath)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string filename = entry.path().filename().string();
                std::string idBase = entry.path().stem().string();
                std::string extension = entry.path().extension().string();
                if (extension == ".ttf" || extension == ".otf") {
                    if (!assets.loadFont(idBase + "_16", path, 16)) { 
                        std::cerr << "GameScene Error: Failed to load font: " << path << " with size 16" << std::endl;
                    }
                }
            }
        }
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
    componentManager->addComponent(playerEntity, RigidbodyComponent{1.0f, true, false, 1.0f});

    Signature playerSignature = entityManager->getSignature(playerEntity);
    playerSignature.set(componentManager->getComponentType<TransformComponent>());
    playerSignature.set(componentManager->getComponentType<VelocityComponent>());
    playerSignature.set(componentManager->getComponentType<SpriteComponent>());
    playerSignature.set(componentManager->getComponentType<RigidbodyComponent>());
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
    componentManager->addComponent(wallEntity, RigidbodyComponent{1.0f, false, true, 1.0f});

    Signature wallSignature = entityManager->getSignature(wallEntity);
    wallSignature.set(componentManager->getComponentType<TransformComponent>());
    wallSignature.set(componentManager->getComponentType<SpriteComponent>());
    wallSignature.set(componentManager->getComponentType<RigidbodyComponent>());
    entityManager->setSignature(wallEntity, wallSignature);
    systemManager->entitySignatureChanged(wallEntity, wallSignature);

    InputManager::getInstance().mapAction("MoveLeft", SDL_SCANCODE_A);
    InputManager::getInstance().mapAction("MoveRight", SDL_SCANCODE_D);
    InputManager::getInstance().mapAction("MoveUp", SDL_SCANCODE_W);
    InputManager::getInstance().mapAction("MoveDown", SDL_SCANCODE_S);

    InputManager::getInstance().mapAction("PlaySound", SDL_SCANCODE_SPACE);
}

GameScene::~GameScene() {
}

void GameScene::handleInput(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
         if (InputManager::getInstance().isActionPressed("PlaySound")) {
            Mix_Chunk* sound = AssetManager::getInstance().getSound("test");
            if (sound) {
                Mix_PlayChannel(-1, sound, 0);
                std::cout << "Played test sound via event" << std::endl;
            } else {
                std::cerr << "Could not get test sound to play." << std::endl;
            }
        }
    }

    if (event.type == SDL_MOUSEWHEEL) {
        if (event.wheel.y > 0) cameraZoom *= 1.1f;
        if (event.wheel.y < 0) cameraZoom /= 1.1f;
        cameraZoom = std::clamp(cameraZoom, 0.2f, 4.0f);
    }
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

    physicsSystem->update(componentManager.get(), deltaTime);
    movementSystem->update(componentManager.get(), deltaTime);
    collisionSystem->update(componentManager.get(), deltaTime);
    animationSystem->update(deltaTime, *entityManager, *componentManager); // Add AnimationSystem update

    auto& playerTransform = componentManager->getComponent<TransformComponent>(playerEntity);
    if (playerTransform.x < 0) playerTransform.x = 0;
    if (playerTransform.y < 0) playerTransform.y = 0;
    if (playerTransform.x + playerTransform.width > 800) playerTransform.x = 800 - playerTransform.width;
    if (playerTransform.y + playerTransform.height > 600) playerTransform.y = 600 - playerTransform.height;

    float viewportW = 800.0f, viewportH = 600.0f;
    float targetX = playerTransform.x + playerTransform.width / 2.0f - (viewportW / 2.0f) / cameraZoom;
    float targetY = playerTransform.y + playerTransform.height / 2.0f - (viewportH / 2.0f) / cameraZoom;
    float lerp = 0.1f;
    cameraX += (targetX - cameraX) * lerp;
    cameraY += (targetY - cameraY) * lerp;
}

void GameScene::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderSetScale(renderer, cameraZoom, cameraZoom);

    renderSystem->update(renderer, componentManager.get(), cameraX, cameraY);

    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    SDL_RenderPresent(renderer);
}
