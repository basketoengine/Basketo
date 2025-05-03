#include "DevModeScene.h"
#include "imgui.h"
#include "../../vendor/imgui/backends/imgui_impl_sdl2.h"
#include "../../vendor/imgui/backends/imgui_impl_sdlrenderer2.h"
#include <iostream>
#include <set> 

DevModeScene::DevModeScene(SDL_Renderer* ren, SDL_Window* win) : renderer(ren), window(win) {
    std::cout << "Entering Dev Mode Scene" << std::endl;
    // ImGui is initialized in Game::init

    // Initialize ECS managers
    entityManager = std::make_unique<EntityManager>();
    componentManager = std::make_unique<ComponentManager>();
    systemManager = std::make_unique<SystemManager>();

    // Register components
    componentManager->registerComponent<TransformComponent>();
    componentManager->registerComponent<SpriteComponent>();

    renderSystem = systemManager->registerSystem<RenderSystem>();
    Signature renderSig;
    renderSig.set(componentManager->getComponentType<TransformComponent>());
    renderSig.set(componentManager->getComponentType<SpriteComponent>());
    systemManager->setSignature<RenderSystem>(renderSig);

    AssetManager& assets = AssetManager::getInstance();
    if (!assets.loadTexture("logo", "../assets/Image/logo.png")) {
        std::cerr << "DevModeScene Error: Failed to load default logo texture!" << std::endl;
    }
     if (!assets.loadTexture("player", "../assets/Image/player.png")) {
        std::cerr << "DevModeScene Error: Failed to load default player texture!" << std::endl;
    }
}

DevModeScene::~DevModeScene() {
    std::cout << "Exiting Dev Mode Scene" << std::endl;
    // ImGui is shutdown in Game::clean
    // ECS managers clean up automatically via unique_ptr
}

void DevModeScene::handleInput() {
    // ImGui_ImplSDL2_ProcessEvent handles SDL events for ImGui
    // This is called from Game::handleEvents
}

void DevModeScene::update(float deltaTime) {
    // Update scene logic here (if any)
    // Could update ECS systems if needed, e.g., movementSystem->update(...)
}

void DevModeScene::render() {

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);

    renderSystem->update(renderer, componentManager.get());

    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Begin("Entity Spawner");
    ImGui::InputFloat("Position X", &spawnPosX);
    ImGui::InputFloat("Position Y", &spawnPosY);
    ImGui::InputFloat("Size Width", &spawnSizeW);
    ImGui::InputFloat("Size Height", &spawnSizeH);
    ImGui::Checkbox("Add Sprite", &spawnAddSprite);
    if (spawnAddSprite) {
        ImGui::InputText("Texture ID", spawnTextureId, IM_ARRAYSIZE(spawnTextureId));
    }

    if (ImGui::Button("Spawn Entity")) {
        Entity newEntity = entityManager->createEntity();

        TransformComponent transform;
        transform.x = spawnPosX;
        transform.y = spawnPosY;
        transform.width = spawnSizeW;
        transform.height = spawnSizeH;
        componentManager->addComponent(newEntity, transform);

        Signature entitySignature;
        entitySignature.set(componentManager->getComponentType<TransformComponent>());

        if (spawnAddSprite) {
            std::string textureIdStr(spawnTextureId);

            if (AssetManager::getInstance().getTexture(textureIdStr)) {
                 componentManager->addComponent(newEntity, SpriteComponent{textureIdStr});
                 entitySignature.set(componentManager->getComponentType<SpriteComponent>());
            } else {
                 std::cerr << "Spawn Warning: Texture ID '" << textureIdStr << "' not found in AssetManager. Sprite not added." << std::endl;
            }
        }

        entityManager->setSignature(newEntity, entitySignature);
        systemManager->entitySignatureChanged(newEntity, entitySignature);

        std::cout << "Spawned Entity " << newEntity << " at (" << spawnPosX << "," << spawnPosY << ")" << std::endl;
    }
    ImGui::End(); 

    ImGui::Begin("Hierarchy");
    ImGui::Text("Entities:");
    ImGui::Separator();
    const auto& activeEntities = entityManager->getActiveEntities();
    for (Entity entity : activeEntities) {
        std::string label = "Entity " + std::to_string(entity);
        if (ImGui::Selectable(label.c_str(), selectedEntity == entity)) {
            selectedEntity = entity;
            inspectorTextureIdBuffer[0] = '\0';
        }
    }
    if (ImGui::Button("Deselect")) {
        selectedEntity = NO_ENTITY_SELECTED;
        inspectorTextureIdBuffer[0] = '\0';
    }
    ImGui::End(); 

    ImGui::Begin("Inspector");
    if (selectedEntity != NO_ENTITY_SELECTED) {
        ImGui::Text("Selected Entity: %u", selectedEntity);
        ImGui::Separator();

        if (componentManager->hasComponent<TransformComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Transform Component")) {
                auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                ImGui::DragFloat("Position X##Transform", &transform.x, 1.0f);
                ImGui::DragFloat("Position Y##Transform", &transform.y, 1.0f);
                ImGui::DragFloat("Width##Transform", &transform.width, 1.0f, 0.0f); 
                ImGui::DragFloat("Height##Transform", &transform.height, 1.0f, 0.0f); 
            }
        } else {
            ImGui::TextDisabled("No Transform Component");
        }

        if (componentManager->hasComponent<SpriteComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Sprite Component")) {
                auto& sprite = componentManager->getComponent<SpriteComponent>(selectedEntity);

                if (inspectorTextureIdBuffer[0] == '\0') {
                    strncpy(inspectorTextureIdBuffer, sprite.textureId.c_str(), IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1);
                    inspectorTextureIdBuffer[IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1] = '\0';
                }

                if (ImGui::InputText("Texture ID##Sprite", inspectorTextureIdBuffer, IM_ARRAYSIZE(inspectorTextureIdBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    std::string newTextureId(inspectorTextureIdBuffer);
                    if (AssetManager::getInstance().getTexture(newTextureId)) {
                        sprite.textureId = newTextureId;
                    } else {
                        std::cerr << "Inspector Warning: Texture ID '" << newTextureId << "' not found. Reverting." << std::endl;
                        strncpy(inspectorTextureIdBuffer, sprite.textureId.c_str(), IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1);
                        inspectorTextureIdBuffer[IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1] = '\0';
                    }
                }
                SDL_Texture* tex = AssetManager::getInstance().getTexture(sprite.textureId);
                if (tex) {
                    ImGui::Text("Current Texture: %s", sprite.textureId.c_str());
                }
            }
        } else {
            ImGui::TextDisabled("No Sprite Component");
        }

        // need to add inspectors for other components here...

    } else {
        ImGui::Text("No entity selected.");
    }
    ImGui::End();

    //Show another simple window (Optional)
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // --- Render ImGui ---
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);
}
