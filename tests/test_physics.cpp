#include <iostream>
#include <SDL2/SDL.h>
#include "../src/ecs/EntityManager.h"
#include "../src/ecs/ComponentManager.h"
#include "../src/ecs/SystemManager.h"
#include "../src/ecs/components/TransformComponent.h"
#include "../src/ecs/components/VelocityComponent.h"
#include "../src/ecs/components/RigidbodyComponent.h"
#include "../src/ecs/systems/PhysicsSystem.h"
#include "../src/ecs/systems/MovementSystem.h"
#include "../src/ecs/systems/CollisionSystem.h"
#include "../src/Physics.h"

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Physics Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 400, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        std::cerr << "SDL_CreateWindow/Renderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    auto entityManager = std::make_unique<EntityManager>();
    auto componentManager = std::make_unique<ComponentManager>();
    auto systemManager = std::make_unique<SystemManager>();

    componentManager->registerComponent<TransformComponent>();
    componentManager->registerComponent<VelocityComponent>();
    componentManager->registerComponent<RigidbodyComponent>();

    auto movementSystem = systemManager->registerSystem<MovementSystem>();
    auto physicsSystem = systemManager->registerSystem<PhysicsSystem>();
    auto collisionSystem = systemManager->registerSystem<CollisionSystem>();

    // Set system signatures
    Signature moveSig;
    moveSig.set(componentManager->getComponentType<TransformComponent>());
    moveSig.set(componentManager->getComponentType<VelocityComponent>());
    systemManager->setSignature<MovementSystem>(moveSig);

    Signature physicsSig;
    physicsSig.set(componentManager->getComponentType<VelocityComponent>());
    physicsSig.set(componentManager->getComponentType<RigidbodyComponent>());
    systemManager->setSignature<PhysicsSystem>(physicsSig);

    Signature collisionSig;
    collisionSig.set(componentManager->getComponentType<TransformComponent>());
    collisionSig.set(componentManager->getComponentType<RigidbodyComponent>());
    systemManager->setSignature<CollisionSystem>(collisionSig);

    // Create player (dynamic)
    Entity player = entityManager->createEntity();
    TransformComponent playerTransform{100, 0, 32, 32, 0.0f, 0};
    VelocityComponent playerVelocity{0, 0};
    RigidbodyComponent playerRb{1.0f, true, false, 1.0f, 0.0f, false};
    componentManager->addComponent(player, playerTransform);
    componentManager->addComponent(player, playerVelocity);
    componentManager->addComponent(player, playerRb);
    Signature playerSig;
    playerSig.set(componentManager->getComponentType<TransformComponent>());
    playerSig.set(componentManager->getComponentType<VelocityComponent>());
    playerSig.set(componentManager->getComponentType<RigidbodyComponent>());
    entityManager->setSignature(player, playerSig);
    systemManager->entitySignatureChanged(player, playerSig);

    // Create wall (static)
    Entity wall = entityManager->createEntity();
    TransformComponent wallTransform{100, 200, 128, 32, 0.0f, 0};
    RigidbodyComponent wallRb{1.0f, false, true, 1.0f, 0.0f, false};
    componentManager->addComponent(wall, wallTransform);
    componentManager->addComponent(wall, wallRb);
    Signature wallSig;
    wallSig.set(componentManager->getComponentType<TransformComponent>());
    wallSig.set(componentManager->getComponentType<RigidbodyComponent>());
    entityManager->setSignature(wall, wallSig);
    systemManager->entitySignatureChanged(wall, wallSig);

    bool running = true;
    SDL_Event event;
    float deltaTime = 1.0f / 60.0f;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }
        physicsSystem->update(componentManager.get(), deltaTime);
        movementSystem->update(componentManager.get(), deltaTime);
        collisionSystem->update(componentManager.get(), deltaTime);

        // Render
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        // Draw wall
        auto& wallT = componentManager->getComponent<TransformComponent>(wall);
        SDL_Rect wallRect = { (int)wallT.x, (int)wallT.y, (int)wallT.width, (int)wallT.height };
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
        SDL_RenderFillRect(renderer, &wallRect);
        // Draw player
        auto& playerT = componentManager->getComponent<TransformComponent>(player);
        SDL_Rect playerRect = { (int)playerT.x, (int)playerT.y, (int)playerT.width, (int)playerT.height };
        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
        SDL_RenderFillRect(renderer, &playerRect);
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
