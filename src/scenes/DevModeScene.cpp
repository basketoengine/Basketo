#include "DevModeScene.h"
#include "imgui.h"
#include "../../vendor/imgui/backends/imgui_impl_sdl2.h"
#include "../../vendor/imgui/backends/imgui_impl_sdlrenderer2.h"
#include <iostream>
#include <set>
#include <fstream> // For file I/O
#include <string>
#include "../../vendor/nlohmann/json.hpp" // Corrected spelling
#include "imgui_internal.h" // For ImGui::IsWindowHovered
#include <SDL2/SDL_rect.h> // For SDL_Rect

// Include component headers needed for serialization/deserialization
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"

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

void DevModeScene::handleInput(SDL_Event& event) {
    // Let ImGui handle its events first
    ImGui_ImplSDL2_ProcessEvent(&event);

    // Check if ImGui wants to capture mouse input
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        // If ImGui is using the mouse, stop dragging if it was active
        if (isDragging) {
            isDragging = false;
            // Optionally apply final snap position here if needed
        }
        return; // Don't process viewport interaction if ImGui has focus
    }

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                bool clickedOnExistingSelection = false;
                if (selectedEntity != NO_ENTITY_SELECTED && isMouseOverEntity(mouseX, mouseY, selectedEntity)) {
                    // Start dragging the currently selected entity
                    isDragging = true;
                    dragStartMouseX = mouseX;
                    dragStartMouseY = mouseY;
                    auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                    dragStartEntityX = transform.x;
                    dragStartEntityY = transform.y;
                    clickedOnExistingSelection = true;
                }

                if (!clickedOnExistingSelection) {
                    // If not clicking the selected entity, check others for selection/drag start
                    Entity clickedEntity = NO_ENTITY_SELECTED;
                    // Iterate in reverse order potentially (top-most rendered first? depends on render order)
                    const auto& activeEntities = entityManager->getActiveEntities();
                    for (auto it = activeEntities.rbegin(); it != activeEntities.rend(); ++it) {
                         Entity entity = *it;
                         if (isMouseOverEntity(mouseX, mouseY, entity)) {
                             clickedEntity = entity;
                             break; // Found the top-most entity under the mouse
                         }
                    }

                    selectedEntity = clickedEntity; // Select the clicked entity (or deselect if none)
                    inspectorTextureIdBuffer[0] = '\0'; // Reset inspector buffer on new selection

                    if (selectedEntity != NO_ENTITY_SELECTED) {
                        // Start dragging the newly selected entity
                        isDragging = true;
                        dragStartMouseX = mouseX;
                        dragStartMouseY = mouseY;
                        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                        dragStartEntityX = transform.x;
                        dragStartEntityY = transform.y;
                    }
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT && isDragging) {
                isDragging = false;
                // Apply final position, especially if snapping
                if (snapToGrid && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                    auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                    transform.x = std::roundf(transform.x / gridSize) * gridSize;
                    transform.y = std::roundf(transform.y / gridSize) * gridSize;
                }
            }
            break;

        case SDL_MOUSEMOTION:
            if (isDragging && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                int deltaX = mouseX - dragStartMouseX;
                int deltaY = mouseY - dragStartMouseY;

                auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                float newX = dragStartEntityX + deltaX;
                float newY = dragStartEntityY + deltaY;

                if (snapToGrid) {
                    // Snap during drag for visual feedback
                    transform.x = std::roundf(newX / gridSize) * gridSize;
                    transform.y = std::roundf(newY / gridSize) * gridSize;
                } else {
                    transform.x = newX;
                    transform.y = newY;
                }
            }
            break;
    }
}

void DevModeScene::update(float deltaTime) {
    // No dragging logic needed here currently, handled in handleInput
}

void DevModeScene::render() {

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);

    // --- Render Grid (Optional) ---
    // Example: Draw a light gray grid
    if (snapToGrid) { // Only draw if snapping is enabled
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); // Light gray
        int screenWidth, screenHeight;
        SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight);
        for (float x = 0; x < screenWidth; x += gridSize) {
            SDL_RenderDrawLineF(renderer, x, 0, x, (float)screenHeight);
        }
        for (float y = 0; y < screenHeight; y += gridSize) {
            SDL_RenderDrawLineF(renderer, 0, y, (float)screenWidth, y);
        }
    }

    renderSystem->update(renderer, componentManager.get());

    // --- Render Selection Box ---
    if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
        SDL_Rect selectionRect = {
            static_cast<int>(transform.x),
            static_cast<int>(transform.y),
            static_cast<int>(transform.width),
            static_cast<int>(transform.height)
        };

        // Set color for selection box (e.g., white)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &selectionRect);
    }

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

    ImGui::Begin("Scene Controls");
    ImGui::InputText("Filename", sceneFilePath, sizeof(sceneFilePath));
    if (ImGui::Button("Save Scene")) {
        saveScene(sceneFilePath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene")) {
        loadScene(sceneFilePath);
    }
    ImGui::Separator();
    ImGui::Checkbox("Snap to Grid", &snapToGrid);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100); // Adjust width as needed
    ImGui::DragFloat("Grid Size", &gridSize, 1.0f, 1.0f, 256.0f); // Min 1, Max 256
    ImGui::End();

    // --- Asset Browser Panel ---
    static std::string selectedTextureId;
    static std::string selectedSoundId;
    static bool previewTexture = false;
    static bool previewSound = false;
    static char assignTextureMsg[128] = "";
    static char assignSoundMsg[128] = "";
    ImGui::Begin("Asset Browser");
    if (ImGui::BeginTabBar("AssetsTabBar")) {
        // --- Texture Tab ---
        if (ImGui::BeginTabItem("Textures")) {
            for (const auto& [id, tex] : AssetManager::getInstance().getAllTextures()) {
                ImGui::PushID(id.c_str());
                if (ImGui::Selectable(id.c_str(), selectedTextureId == id)) {
                    selectedTextureId = id;
                    previewTexture = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Preview")) {
                    selectedTextureId = id;
                    previewTexture = true;
                }
                if (selectedTextureId == id && previewTexture && tex) {
                    int w, h;
                    SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                    ImGui::Text("%s (%dx%d)", id.c_str(), w, h);
                    ImGui::Image((ImTextureID)tex, ImVec2(64, 64));
                    if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<SpriteComponent>(selectedEntity)) {
                        if (ImGui::Button("Assign to Selected SpriteComponent")) {
                            auto& sprite = componentManager->getComponent<SpriteComponent>(selectedEntity);
                            sprite.textureId = id;
                            snprintf(assignTextureMsg, sizeof(assignTextureMsg), "Assigned '%s' to entity %u", id.c_str(), selectedEntity);
                        }
                        if (assignTextureMsg[0]) ImGui::TextColored(ImVec4(0,1,0,1), "%s", assignTextureMsg);
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        // --- Audio Tab ---
        if (ImGui::BeginTabItem("Audio")) {
            for (const auto& [id, chunk] : AssetManager::getInstance().getAllSounds()) {
                ImGui::PushID(id.c_str());
                if (ImGui::Selectable(id.c_str(), selectedSoundId == id)) {
                    selectedSoundId = id;
                    previewSound = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Play")) {
                    selectedSoundId = id;
                    previewSound = true;
                    Mix_PlayChannel(-1, chunk, 0);
                    snprintf(assignSoundMsg, sizeof(assignSoundMsg), "Playing '%s'", id.c_str());
                }
                if (selectedSoundId == id && previewSound && chunk) {
                    ImGui::Text("%s (Mix_Chunk*)", id.c_str());
                    if (assignSoundMsg[0]) ImGui::TextColored(ImVec4(0,1,0,1), "%s", assignSoundMsg);
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
    ImGui::EndTabBar();
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

void DevModeScene::saveScene(const std::string& filepath) {
    nlohmann::json sceneJson;
    sceneJson["entities"] = nlohmann::json::array();

    std::cout << "Saving scene to " << filepath << "..." << std::endl;

    const auto& activeEntities = entityManager->getActiveEntities();
    for (Entity entity : activeEntities) {
        nlohmann::json entityJson;
        entityJson["id_saved"] = entity;
        entityJson["components"] = nlohmann::json::object();

        if (componentManager->hasComponent<TransformComponent>(entity)) {
            entityJson["components"]["TransformComponent"] = componentManager->getComponent<TransformComponent>(entity);
        }
        if (componentManager->hasComponent<SpriteComponent>(entity)) {
            entityJson["components"]["SpriteComponent"] = componentManager->getComponent<SpriteComponent>(entity);
        }

        if (!entityJson["components"].empty()) {
             sceneJson["entities"].push_back(entityJson);
        }
    }

    std::ofstream outFile(filepath);
    if (outFile.is_open()) {
        outFile << sceneJson.dump(4);
        outFile.close();
        std::cout << "Scene saved successfully." << std::endl;
    } else {
        std::cerr << "Error: Could not open file " << filepath << " for writing!" << std::endl;
    }
}

void DevModeScene::loadScene(const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file " << filepath << " for reading!" << std::endl;
        return;
    }

    nlohmann::json sceneJson;
    try {
        inFile >> sceneJson;
        inFile.close();
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Error: Failed to parse scene file " << filepath << ". " << e.what() << std::endl;
        inFile.close();
        return;
    }

    std::cout << "Loading scene from " << filepath << "..." << std::endl;

    // --- Clear Existing Scene ---
    // Iterate and destroy existing entities
    std::set<Entity> entitiesToDestroy = entityManager->getActiveEntities();
    for (Entity entity : entitiesToDestroy) {
        entityManager->destroyEntity(entity);
        componentManager->entityDestroyed(entity);
    }
    if (!entityManager->getActiveEntities().empty()) {
        std::cerr << "Warning: Not all entities were destroyed during scene load cleanup!" << std::endl;
    }

    selectedEntity = NO_ENTITY_SELECTED;
    inspectorTextureIdBuffer[0] = '\0';

    // --- Load Entities ---
    if (!sceneJson.contains("entities") || !sceneJson["entities"].is_array()) {
         std::cerr << "Error: Scene file format incorrect. Missing 'entities' array." << std::endl;
         return;
    }

    const auto& entitiesJson = sceneJson["entities"];
    for (const auto& entityJson : entitiesJson) {
        Entity newEntity = entityManager->createEntity();

        if (!entityJson.contains("components") || !entityJson["components"].is_object()) {
            std::cerr << "Warning: Entity definition missing 'components' object. Skipping." << std::endl;
            entityManager->destroyEntity(newEntity);
            continue;
        }

        Signature entitySignature;

        const auto& componentsJson = entityJson["components"];
        for (auto it = componentsJson.begin(); it != componentsJson.end(); ++it) {
            const std::string& componentType = it.key();
            const nlohmann::json& componentData = it.value();

            try {
                if (componentType == "TransformComponent") {
                    TransformComponent comp;
                    from_json(componentData, comp);
                    componentManager->addComponent(newEntity, comp);
                    entitySignature.set(componentManager->getComponentType<TransformComponent>());
                } else if (componentType == "SpriteComponent") {
                    SpriteComponent comp;
                    from_json(componentData, comp);
                    componentManager->addComponent(newEntity, comp);
                    entitySignature.set(componentManager->getComponentType<SpriteComponent>());
                    if (!AssetManager::getInstance().getTexture(comp.textureId)) {
                         std::cerr << "Warning: Texture '" << comp.textureId
                                   << "' needed by loaded entity " << newEntity
                                   << " is not loaded in AssetManager! Ensure it's loaded before loading the scene." << std::endl;
                    }
                } else {
                    std::cerr << "Warning: Unknown component type '" << componentType << "' encountered during loading for entity." << std::endl;
                }
            } catch (nlohmann::json::exception& e) {
                 std::cerr << "Error parsing component '" << componentType << "' for entity. " << e.what() << std::endl;
            }
        }
        entityManager->setSignature(newEntity, entitySignature);
        systemManager->entitySignatureChanged(newEntity, entitySignature);
    }
    std::cout << "Scene loaded successfully." << std::endl;
}

bool DevModeScene::isMouseOverEntity(int mouseX, int mouseY, Entity entity) {
    if (entity == NO_ENTITY_SELECTED || !componentManager->hasComponent<TransformComponent>(entity)) {
        return false;
    }
    auto& transform = componentManager->getComponent<TransformComponent>(entity);
    return (mouseX >= transform.x && mouseX < (transform.x + transform.width) &&
            mouseY >= transform.y && mouseY < (transform.y + transform.height));
}
