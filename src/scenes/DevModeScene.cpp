#include "DevModeScene.h"
#include "imgui.h"
#include "../../vendor/imgui/backends/imgui_impl_sdl2.h"
#include "../../vendor/imgui/backends/imgui_impl_sdlrenderer2.h"
#include <iostream>
#include <set>
#include <fstream>
#include <string>
#include "../../vendor/nlohmann/json.hpp" 
#include "imgui_internal.h"
#include <SDL2/SDL_rect.h>
#include <vector>
#include <utility> 

#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"

DevModeScene::DevModeScene(SDL_Renderer* ren, SDL_Window* win) : renderer(ren), window(win) {
    std::cout << "Entering Dev Mode Scene" << std::endl;

    entityManager = std::make_unique<EntityManager>();
    componentManager = std::make_unique<ComponentManager>();
    systemManager = std::make_unique<SystemManager>();

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
    if (!assets.loadTexture("world", "../assets/Image/world.jpg")) {
        std::cerr << "DevModeScene Error: Failed to load default world texture!" << std::endl;
    }
}

DevModeScene::~DevModeScene() {
    std::cout << "Exiting Dev Mode Scene" << std::endl;
}

void DevModeScene::handleInput(SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    ImGuiIO& io = ImGui::GetIO();

    if (isPlaying) {
        return; 
    }

    if (io.WantCaptureMouse) {
        if (isDragging || isResizing) {
            isDragging = false; 
            isResizing = false;
            activeHandle = ResizeHandle::NONE;
        }
        return; 
    }

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                    auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                    activeHandle = getHandleAtPosition(mouseX, mouseY, transform);

                    if (activeHandle != ResizeHandle::NONE) {

                        isResizing = true;
                        isDragging = false; 
                        dragStartMouseX = mouseX;
                        dragStartMouseY = mouseY;
                        dragStartEntityX = transform.x;
                        dragStartEntityY = transform.y;
                        dragStartWidth = transform.width;
                        dragStartHeight = transform.height;
                    } else if (isMouseOverEntity(mouseX, mouseY, selectedEntity)) {
                        
                        isDragging = true;
                        isResizing = false; 
                        dragStartMouseX = mouseX;
                        dragStartMouseY = mouseY;
                        dragStartEntityX = transform.x;
                        dragStartEntityY = transform.y;
                    } else {
                        
                        isDragging = false;
                        isResizing = false;
                        activeHandle = ResizeHandle::NONE;
                        Entity clickedEntity = NO_ENTITY_SELECTED;
                        const auto& activeEntities = entityManager->getActiveEntities();
                        for (auto it = activeEntities.rbegin(); it != activeEntities.rend(); ++it) {
                             Entity entity = *it;
                             if (isMouseOverEntity(mouseX, mouseY, entity)) {
                                 clickedEntity = entity;
                                 break;
                             }
                        }
                        selectedEntity = clickedEntity;
                        inspectorTextureIdBuffer[0] = '\0';
                        if (selectedEntity != NO_ENTITY_SELECTED && isMouseOverEntity(mouseX, mouseY, selectedEntity)) {
                            isDragging = true;
                            auto& newTransform = componentManager->getComponent<TransformComponent>(selectedEntity);
                            dragStartMouseX = mouseX;
                            dragStartMouseY = mouseY;
                            dragStartEntityX = newTransform.x;
                            dragStartEntityY = newTransform.y;
                        }
                    }
                } else {
                     isDragging = false;
                     isResizing = false;
                     activeHandle = ResizeHandle::NONE;
                     Entity clickedEntity = NO_ENTITY_SELECTED;
                     const auto& activeEntities = entityManager->getActiveEntities();
                     for (auto it = activeEntities.rbegin(); it != activeEntities.rend(); ++it) {
                          Entity entity = *it;
                          if (isMouseOverEntity(mouseX, mouseY, entity)) {
                              clickedEntity = entity;
                              break;
                          }
                     }
                     selectedEntity = clickedEntity;
                     inspectorTextureIdBuffer[0] = '\0';
                     if (selectedEntity != NO_ENTITY_SELECTED && isMouseOverEntity(mouseX, mouseY, selectedEntity)) {
                         isDragging = true;
                         auto& newTransform = componentManager->getComponent<TransformComponent>(selectedEntity);
                         dragStartMouseX = mouseX;
                         dragStartMouseY = mouseY;
                         dragStartEntityX = newTransform.x;
                         dragStartEntityY = newTransform.y;
                     }
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
             if (event.button.button == SDL_BUTTON_LEFT) {
                 if (isDragging) {
                     isDragging = false;
                     if (snapToGrid && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                         auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                         transform.x = std::roundf(transform.x / gridSize) * gridSize;
                         transform.y = std::roundf(transform.y / gridSize) * gridSize;
                     }
                 }
                 if (isResizing) {
                     isResizing = false;
                     activeHandle = ResizeHandle::NONE;
                     if (snapToGrid && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                         auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                         transform.x = std::roundf(transform.x / gridSize) * gridSize;
                         transform.y = std::roundf(transform.y / gridSize) * gridSize;
                         transform.width = std::max(gridSize, std::roundf(transform.width / gridSize) * gridSize);
                         transform.height = std::max(gridSize, std::roundf(transform.height / gridSize) * gridSize);
                     }
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
                     transform.x = std::roundf(newX / gridSize) * gridSize;
                     transform.y = std::roundf(newY / gridSize) * gridSize;
                 } else {
                     transform.x = newX;
                     transform.y = newY;
                 }

             } else if (isResizing && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                 auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                 int deltaX = mouseX - dragStartMouseX;
                 int deltaY = mouseY - dragStartMouseY;

                 float newX = transform.x;
                 float newY = transform.y;
                 float newWidth = transform.width;
                 float newHeight = transform.height;

                 switch (activeHandle) {
                     case ResizeHandle::TOP_LEFT:
                         newX = dragStartEntityX + deltaX;
                         newY = dragStartEntityY + deltaY;
                         newWidth = dragStartWidth - deltaX;
                         newHeight = dragStartHeight - deltaY;
                         break;
                     case ResizeHandle::TOP_RIGHT:
                         newY = dragStartEntityY + deltaY;
                         newWidth = dragStartWidth + deltaX;
                         newHeight = dragStartHeight - deltaY;
                         break;
                     case ResizeHandle::BOTTOM_LEFT:
                         newX = dragStartEntityX + deltaX;
                         newWidth = dragStartWidth - deltaX;
                         newHeight = dragStartHeight + deltaY;
                         break;
                     case ResizeHandle::BOTTOM_RIGHT:
                         newWidth = dragStartWidth + deltaX;
                         newHeight = dragStartHeight + deltaY;
                         break;
                     case ResizeHandle::NONE:
                         break; 
                 }

                 if (newWidth < HANDLE_SIZE) newWidth = HANDLE_SIZE;
                 if (newHeight < HANDLE_SIZE) newHeight = HANDLE_SIZE;

                 if (snapToGrid) {
                     float snappedX = std::roundf(newX / gridSize) * gridSize;
                     float snappedY = std::roundf(newY / gridSize) * gridSize;
                     float snappedWidth = std::max(gridSize, std::roundf(newWidth / gridSize) * gridSize);
                     float snappedHeight = std::max(gridSize, std::roundf(newHeight / gridSize) * gridSize);

                     if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::BOTTOM_LEFT) {
                         snappedX += (newWidth - snappedWidth); 
                     }
                     if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::TOP_RIGHT) {
                         snappedY += (newHeight - snappedHeight);
                     }

                     transform.x = snappedX;
                     transform.y = snappedY;
                     transform.width = snappedWidth;
                     transform.height = snappedHeight;

                 } else {
                     transform.x = newX;
                     transform.y = newY;
                     transform.width = newWidth;
                     transform.height = newHeight;
                 }
             }
            break;
    }
}

void DevModeScene::update(float deltaTime) {
    if (isPlaying) {
    } else {
    }
}

void DevModeScene::render() {

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);

    renderSystem->update(renderer, componentManager.get());

    if (!isPlaying) {
        if (snapToGrid) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            int screenWidth, screenHeight;
            SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight);
            for (float x = 0; x < screenWidth; x += gridSize) {
                SDL_RenderDrawLineF(renderer, x, 0, x, (float)screenHeight);
            }
            for (float y = 0; y < screenHeight; y += gridSize) {
                SDL_RenderDrawLineF(renderer, 0, y, (float)screenWidth, y);
            }
        }

        if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
            auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
            
            SDL_Rect selectionRect = {
                static_cast<int>(transform.x), static_cast<int>(transform.y),
                static_cast<int>(transform.width), static_cast<int>(transform.height)
            };
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(renderer, &selectionRect);

            SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
            for (const auto& handlePair : getResizeHandles(transform)) {
                SDL_RenderFillRect(renderer, &handlePair.second);
            }
        }
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
            if (ImGui::CollapsingHeader("Transform Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                ImGui::DragFloat("Position X##Transform", &transform.x, 1.0f);
                ImGui::DragFloat("Position Y##Transform", &transform.y, 1.0f);
                ImGui::DragFloat("Width##Transform", &transform.width, 1.0f, HANDLE_SIZE);
                ImGui::DragFloat("Height##Transform", &transform.height, 1.0f, HANDLE_SIZE);
                ImGui::DragFloat("Rotation##Transform", &transform.rotation, 1.0f, -360.0f, 360.0f);
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

    } else {
        ImGui::Text("No entity selected.");
    }
    ImGui::End();

    ImGui::Begin("Scene Controls");
    if (isPlaying) {
        if (ImGui::Button("Stop")) {
            isPlaying = false;
            std::cout << "Stopping Play Mode. Reloading scene..." << std::endl;
            loadScene(sceneFilePath);
        }
    } else {
        if (ImGui::Button("Play")) {
            isPlaying = true;
            std::cout << "Starting Play Mode..." << std::endl;

            selectedEntity = NO_ENTITY_SELECTED;
            isDragging = false; 
            inspectorTextureIdBuffer[0] = '\0'; 
        }
    }
    ImGui::Separator();

    ImGui::BeginDisabled(isPlaying);
    ImGui::InputText("Filename", sceneFilePath, sizeof(sceneFilePath));
    if (ImGui::Button("Save Scene")) {
        saveScene(sceneFilePath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Scene")) {
        loadScene(sceneFilePath);
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    ImGui::BeginDisabled(isPlaying);
    ImGui::Checkbox("Snap to Grid", &snapToGrid);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::DragFloat("Grid Size", &gridSize, 1.0f, 1.0f, 256.0f);
    ImGui::EndDisabled();

    ImGui::End();

    ImGui::Begin("Asset Browser");
    static std::string selectedTextureId;
    static std::string selectedSoundId;
    static bool previewTexture = false;
    static bool previewSound = false;
    static char assignTextureMsg[128] = "";
    static char assignSoundMsg[128] = "";
    if (ImGui::BeginTabBar("AssetsTabBar")) {
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

    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

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

    std::set<Entity> entitiesToDestroy = entityManager->getActiveEntities();
    for (Entity entity : entitiesToDestroy) {
        entityManager->destroyEntity(entity);
        componentManager->entityDestroyed(entity);
        systemManager->entityDestroyed(entity);
    }
    if (!entityManager->getActiveEntities().empty()) {
        std::cerr << "Warning: Not all entities were destroyed during scene load cleanup!" << std::endl;
    }

    selectedEntity = NO_ENTITY_SELECTED;
    inspectorTextureIdBuffer[0] = '\0';

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

std::vector<std::pair<ResizeHandle, SDL_Rect>> DevModeScene::getResizeHandles(const TransformComponent& transform) {
    std::vector<std::pair<ResizeHandle, SDL_Rect>> handles;
    int halfHandle = HANDLE_SIZE / 2;

    handles.push_back({ResizeHandle::TOP_LEFT, {
        static_cast<int>(transform.x - halfHandle),
        static_cast<int>(transform.y - halfHandle),
        HANDLE_SIZE, HANDLE_SIZE
    }});
    handles.push_back({ResizeHandle::TOP_RIGHT, {
        static_cast<int>(transform.x + transform.width - halfHandle),
        static_cast<int>(transform.y - halfHandle),
        HANDLE_SIZE, HANDLE_SIZE
    }});
    handles.push_back({ResizeHandle::BOTTOM_LEFT, {
        static_cast<int>(transform.x - halfHandle),
        static_cast<int>(transform.y + transform.height - halfHandle),
        HANDLE_SIZE, HANDLE_SIZE
    }});
    handles.push_back({ResizeHandle::BOTTOM_RIGHT, {
        static_cast<int>(transform.x + transform.width - halfHandle),
        static_cast<int>(transform.y + transform.height - halfHandle),
        HANDLE_SIZE, HANDLE_SIZE
    }});

    return handles;
}

ResizeHandle DevModeScene::getHandleAtPosition(int mouseX, int mouseY, const TransformComponent& transform) {
    for (const auto& handlePair : getResizeHandles(transform)) {
        const SDL_Rect& rect = handlePair.second;
        if (mouseX >= rect.x && mouseX < (rect.x + rect.w) &&
            mouseY >= rect.y && mouseY < (rect.y + rect.h)) {
            return handlePair.first;
        }
    }
    return ResizeHandle::NONE;
}
