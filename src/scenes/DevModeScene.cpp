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

    // Get raw mouse state relative to the main window
    int rawMouseX, rawMouseY;
    Uint32 mouseButtonState = SDL_GetMouseState(&rawMouseX, &rawMouseY);

    // Calculate mouse coordinates relative to the game viewport
    int viewportMouseX = rawMouseX - gameViewport.x;
    int viewportMouseY = rawMouseY - gameViewport.y;

    // Calculate mouse coordinates in world space
    float worldMouseX = viewportMouseX + cameraX;
    float worldMouseY = viewportMouseY + cameraY;

    // Check if mouse is within the game viewport bounds
    bool mouseInViewport = (rawMouseX >= gameViewport.x && rawMouseX < (gameViewport.x + gameViewport.w) &&
                            rawMouseY >= gameViewport.y && rawMouseY < (gameViewport.y + gameViewport.h));

    // --- Camera Panning Logic ---
    static bool isPanning = false;
    static int panStartX = 0;
    static int panStartY = 0;
    static float panStartCameraX = 0.0f;
    static float panStartCameraY = 0.0f;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE && mouseInViewport && !io.WantCaptureMouse) {
        isPanning = true;
        panStartX = rawMouseX;
        panStartY = rawMouseY;
        panStartCameraX = cameraX;
        panStartCameraY = cameraY;
        SDL_SetRelativeMouseMode(SDL_TRUE); // Optional: Hide cursor during pan
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE) {
        if (isPanning) {
            isPanning = false;
            SDL_SetRelativeMouseMode(SDL_FALSE); // Show cursor again
        }
    }
    if (event.type == SDL_MOUSEMOTION && isPanning) {
        int deltaX = rawMouseX - panStartX;
        int deltaY = rawMouseY - panStartY;
        cameraX = panStartCameraX - deltaX; // Move camera opposite to mouse drag
        cameraY = panStartCameraY - deltaY;
    }
    // --- End Camera Panning ---

    if (isPlaying || isPanning) { // Don't process entity interactions if playing or panning
        return;
    }

    // Only process game viewport interactions if ImGui doesn't want the mouse AND the mouse is inside the viewport
    if (!io.WantCaptureMouse && mouseInViewport) {
        switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                        // Use world-space mouse coordinates for handle checks
                        activeHandle = getHandleAtPosition(worldMouseX, worldMouseY, transform);

                        if (activeHandle != ResizeHandle::NONE) {
                            isResizing = true;
                            isDragging = false;
                            dragStartMouseX = worldMouseX; // Store world coords
                            dragStartMouseY = worldMouseY;
                            dragStartEntityX = transform.x;
                            dragStartEntityY = transform.y;
                            dragStartWidth = transform.width;
                            dragStartHeight = transform.height;
                        } else if (isMouseOverEntity(worldMouseX, worldMouseY, selectedEntity)) { // Use world coords
                            isDragging = true;
                            isResizing = false;
                            dragStartMouseX = worldMouseX; // Store world coords
                            dragStartMouseY = worldMouseY;
                            dragStartEntityX = transform.x;
                            dragStartEntityY = transform.y;
                        } else {
                            // Clicked outside the selected entity, try selecting a new one
                            isDragging = false;
                            isResizing = false;
                            activeHandle = ResizeHandle::NONE;
                            Entity clickedEntity = NO_ENTITY_SELECTED;
                            const auto& activeEntities = entityManager->getActiveEntities();
                            for (auto it = activeEntities.rbegin(); it != activeEntities.rend(); ++it) {
                                 Entity entity = *it;
                                 // Use world coords for selection check
                                 if (isMouseOverEntity(worldMouseX, worldMouseY, entity)) {
                                     clickedEntity = entity;
                                     break;
                                 }
                            }
                            selectedEntity = clickedEntity;
                            inspectorTextureIdBuffer[0] = '\0';

                            if (selectedEntity != NO_ENTITY_SELECTED && isMouseOverEntity(worldMouseX, worldMouseY, selectedEntity)) {
                                isDragging = true;
                                auto& newTransform = componentManager->getComponent<TransformComponent>(selectedEntity);
                                dragStartMouseX = worldMouseX; // Store world coords
                                dragStartMouseY = worldMouseY;
                                dragStartEntityX = newTransform.x;
                                dragStartEntityY = newTransform.y;
                            }
                        }
                    } else {
                         // No entity was selected, try selecting one
                         isDragging = false;
                         isResizing = false;
                         activeHandle = ResizeHandle::NONE;
                         Entity clickedEntity = NO_ENTITY_SELECTED;
                         const auto& activeEntities = entityManager->getActiveEntities();
                         for (auto it = activeEntities.rbegin(); it != activeEntities.rend(); ++it) {
                              Entity entity = *it;
                              // Use world coords for selection check
                              if (isMouseOverEntity(worldMouseX, worldMouseY, entity)) {
                                  clickedEntity = entity;
                                  break;
                              }
                         }
                         selectedEntity = clickedEntity;
                         inspectorTextureIdBuffer[0] = '\0';

                         if (selectedEntity != NO_ENTITY_SELECTED && isMouseOverEntity(worldMouseX, worldMouseY, selectedEntity)) {
                             isDragging = true;
                             auto& newTransform = componentManager->getComponent<TransformComponent>(selectedEntity);
                             dragStartMouseX = worldMouseX; // Store world coords
                             dragStartMouseY = worldMouseY;
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
                     float deltaX = worldMouseX - dragStartMouseX;
                     float deltaY = worldMouseY - dragStartMouseY;
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
                     float deltaX = worldMouseX - dragStartMouseX;
                     float deltaY = worldMouseY - dragStartMouseY;

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
    } else {
         if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
             isDragging = false;
             isResizing = false;
             activeHandle = ResizeHandle::NONE;
         }
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

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    const float hierarchyWidth = displaySize.x * 0.18f;
    const float inspectorWidth = displaySize.x * 0.22f;
    const float bottomPanelHeight = displaySize.y * 0.25f;
    const float topToolbarHeight = 40.0f;

    gameViewport = { (int)hierarchyWidth, (int)topToolbarHeight, (int)(displaySize.x - hierarchyWidth - inspectorWidth), (int)(displaySize.y - bottomPanelHeight - topToolbarHeight) };

    const ImGuiWindowFlags fixedPanelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    const ImGuiWindowFlags viewWindowFlags = ImGuiWindowFlags_None; // Define viewWindowFlags

    SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);

    SDL_RenderSetViewport(renderer, &gameViewport);
    renderSystem->update(renderer, componentManager.get(), cameraX, cameraY);

    if (!isPlaying) {
        if (snapToGrid) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            float gridStartX = -fmodf(cameraX, gridSize);
            float gridStartY = -fmodf(cameraY, gridSize);
            for (float x = gridStartX; x < gameViewport.w; x += gridSize) {
                SDL_RenderDrawLineF(renderer, x, 0, x, gameViewport.h);
            }
            for (float y = gridStartY; y < gameViewport.h; y += gridSize) {
                SDL_RenderDrawLineF(renderer, 0, y, gameViewport.w, y);
            }
        }

        if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
            auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
            SDL_Rect selectionRect = {
                static_cast<int>(transform.x - cameraX),
                static_cast<int>(transform.y - cameraY),
                static_cast<int>(transform.width),
                static_cast<int>(transform.height)
            };
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(renderer, &selectionRect);

            SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
            for (const auto& handlePair : getResizeHandles(transform)) {
                SDL_Rect handleRect = handlePair.second;
                handleRect.x -= static_cast<int>(cameraX);
                handleRect.y -= static_cast<int>(cameraY);
                SDL_RenderFillRect(renderer, &handleRect);
            }
        }
    }
    SDL_RenderSetViewport(renderer, NULL);

    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // --- Render Fixed Panels (Toolbar, Hierarchy, Inspector, Bottom) ---
    // Toolbar at the very top, full width
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, topToolbarHeight), ImGuiCond_Always);
    ImGui::Begin("Toolbar", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::Button("Save")) saveScene(sceneFilePath);
    ImGui::SameLine(); if (ImGui::Button("Save As...")) std::cout << "Placeholder: Save As." << std::endl;
    ImGui::SameLine(); if (ImGui::Button("Load")) loadScene(sceneFilePath);
    ImGui::SameLine(); ImGui::PushItemWidth(120); ImGui::InputText("##Filename", sceneFilePath, sizeof(sceneFilePath)); ImGui::PopItemWidth();
    ImGui::SameLine(); if (ImGui::Button("Import...")) std::cout << "Placeholder: Import." << std::endl;
    ImGui::SameLine(); if (ImGui::Button("Export...")) std::cout << "Placeholder: Export." << std::endl;
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    if (isPlaying) { if (ImGui::Button("Stop")) { isPlaying = false; loadScene(sceneFilePath); } }
    else { if (ImGui::Button("Play")) { isPlaying = true; selectedEntity = NO_ENTITY_SELECTED; isDragging = false; isResizing = false; activeHandle = ResizeHandle::NONE; inspectorTextureIdBuffer[0] = '\0'; } }
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    ImGui::BeginDisabled(isPlaying);
    ImGui::Checkbox("Snap", &snapToGrid); ImGui::SameLine(); ImGui::PushItemWidth(60); ImGui::DragFloat("Grid", &gridSize, 1.0f, 1.0f, 256.0f, "%.0f"); ImGui::PopItemWidth();
    ImGui::EndDisabled();
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    ImGui::Checkbox("Show Grid", &showGrid);
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    ImGui::Text("Spawn:"); ImGui::SameLine(); ImGui::PushItemWidth(40); ImGui::InputFloat("X", &spawnPosX, 0, 0, "%.0f"); ImGui::SameLine(); ImGui::InputFloat("Y", &spawnPosY, 0, 0, "%.0f"); ImGui::PopItemWidth(); ImGui::SameLine(); ImGui::PushItemWidth(60); ImGui::InputText("TexID", spawnTextureId, IM_ARRAYSIZE(spawnTextureId)); ImGui::PopItemWidth(); ImGui::SameLine();
    if (ImGui::Button("Spawn")) {
        Entity newEntity = entityManager->createEntity();
        float spawnWorldX = cameraX + gameViewport.w / 2.0f;
        float spawnWorldY = cameraY + gameViewport.h / 2.0f;
        TransformComponent transform; transform.x = spawnWorldX + spawnPosX; transform.y = spawnWorldY + spawnPosY; transform.width = spawnSizeW > 0 ? spawnSizeW : 32; transform.height = spawnSizeH > 0 ? spawnSizeH : 32;
        componentManager->addComponent(newEntity, transform);
        Signature entitySignature; entitySignature.set(componentManager->getComponentType<TransformComponent>());
        std::string textureIdStr(spawnTextureId);
        if (strlen(spawnTextureId) > 0 && AssetManager::getInstance().loadTexture(textureIdStr, textureIdStr)) { componentManager->addComponent(newEntity, SpriteComponent{textureIdStr}); entitySignature.set(componentManager->getComponentType<SpriteComponent>()); }
        else if (strlen(spawnTextureId) > 0) std::cerr << "Spawn Warning: Texture ID '" << textureIdStr << "' not found. Sprite not added." << std::endl;
        entityManager->setSignature(newEntity, entitySignature); systemManager->entitySignatureChanged(newEntity, entitySignature);
    }
    ImGui::End(); // End Toolbar

    // Hierarchy below Toolbar on the left
    ImGui::SetNextWindowPos(ImVec2(0, topToolbarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hierarchyWidth, displaySize.y - topToolbarHeight - bottomPanelHeight), ImGuiCond_Always);
    ImGui::Begin("Hierarchy", nullptr, fixedPanelFlags);
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

    // Inspector below Toolbar on the right
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - inspectorWidth, topToolbarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(inspectorWidth, displaySize.y - topToolbarHeight - bottomPanelHeight), ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr, fixedPanelFlags);
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
        } else ImGui::TextDisabled("No Transform Component");
        if (componentManager->hasComponent<SpriteComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Sprite Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& sprite = componentManager->getComponent<SpriteComponent>(selectedEntity);
                if (inspectorTextureIdBuffer[0] == '\0' || sprite.textureId != inspectorTextureIdBuffer) {
                    strncpy(inspectorTextureIdBuffer, sprite.textureId.c_str(), IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1);
                    inspectorTextureIdBuffer[IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1] = '\0';
                }
                ImGui::Text("Texture ID/Path:");
                if (ImGui::InputText("##SpriteTexturePath", inspectorTextureIdBuffer, IM_ARRAYSIZE(inspectorTextureIdBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    std::string newTexturePath(inspectorTextureIdBuffer);
                    AssetManager& assets = AssetManager::getInstance();
                    if (assets.loadTexture(newTexturePath, newTexturePath)) sprite.textureId = newTexturePath;
                    else {
                        std::cerr << "Inspector Error: Failed to load texture: '" << newTexturePath << "'. Reverting." << std::endl;
                        strncpy(inspectorTextureIdBuffer, sprite.textureId.c_str(), IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1);
                        inspectorTextureIdBuffer[IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1] = '\0';
                    }
                }
                SDL_Texture* tex = AssetManager::getInstance().getTexture(sprite.textureId);
                if (tex) {
                    int w, h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                    ImGui::Text("Current: %s (%dx%d)", sprite.textureId.c_str(), w, h);
                    ImGui::Image((ImTextureID)tex, ImVec2(64, 64));
                } else ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Current texture not loaded!");
                if (ImGui::Button("Browse...##SpriteTexture")) std::cout << "Placeholder: File browser." << std::endl;
            }
        } else ImGui::TextDisabled("No Sprite Component");
    } else ImGui::Text("No entity selected.");
    ImGui::End();

    // Bottom panel at the bottom, full width
    ImGui::SetNextWindowPos(ImVec2(0, displaySize.y - bottomPanelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, bottomPanelHeight), ImGuiCond_Always);
    ImGui::Begin("BottomPanel", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoScrollbar);
    if (ImGui::BeginTabBar("BottomTabs")) {
        if (ImGui::BeginTabItem("Project")) {
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Console")) {
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    // --- Render Dockable Views (Scene View, Game View) ---
    // These will dock into the central space left by the fixed panels
    ImGui::Begin("Scene View", nullptr, viewWindowFlags);
    // Scene View content would go here (if any specific ImGui elements were needed inside it)
    // Currently, the SDL rendering happens outside this Begin/End pair, targeting the viewport.
    ImGui::End(); // Added missing End() for Scene View

    if (show_another_window) {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

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
    AssetManager& assets = AssetManager::getInstance();

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

                    if (!assets.getTexture(comp.textureId)) {
                         std::cout << "LoadScene: Texture '" << comp.textureId << "' not pre-loaded. Attempting dynamic load..." << std::endl;
                         if (!assets.loadTexture(comp.textureId, comp.textureId)) {
                             std::cerr << "LoadScene Warning: Failed to dynamically load texture '" << comp.textureId
                                       << "' needed by loaded entity " << newEntity << "." << std::endl;
                         } else {
                              std::cout << "LoadScene: Successfully loaded texture '" << comp.textureId << "' dynamically." << std::endl;
                         }
                    }

                    componentManager->addComponent(newEntity, comp);
                    entitySignature.set(componentManager->getComponentType<SpriteComponent>());
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

bool DevModeScene::isMouseOverEntity(int worldMouseX, int worldMouseY, Entity entity) {
    if (entity == NO_ENTITY_SELECTED || !componentManager->hasComponent<TransformComponent>(entity)) {
        return false;
    }
    auto& transform = componentManager->getComponent<TransformComponent>(entity);
    return (worldMouseX >= transform.x && worldMouseX < (transform.x + transform.width) &&
            worldMouseY >= transform.y && worldMouseY < (transform.y + transform.height));
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

ResizeHandle DevModeScene::getHandleAtPosition(int worldMouseX, int worldMouseY, const TransformComponent& transform) {
    for (const auto& handlePair : getResizeHandles(transform)) {
        const SDL_Rect& rect = handlePair.second;
        if (worldMouseX >= rect.x && worldMouseX < (rect.x + rect.w) &&
            worldMouseY >= rect.y && worldMouseY < (rect.y + rect.h)) {
            return handlePair.first;
        }
    }
    return ResizeHandle::NONE;
}
