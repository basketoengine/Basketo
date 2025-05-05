#include "DevModeScene.h"
#include "imgui.h"
#include "../../vendor/imgui/backends/imgui_impl_sdl2.h"
#include "../../vendor/imgui/backends/imgui_impl_sdlrenderer2.h"
#include "tinyfiledialogs.h"
#include <filesystem>
#include <iostream>
#include <set>
#include <fstream>
#include <string>
#include <vector>
#include "../../vendor/nlohmann/json.hpp"
#include "imgui_internal.h"
#include <SDL2/SDL_rect.h>
#include <utility>

#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"

// Helper function to get filename from path
std::string getFilenameFromPath(const std::string& path) {
    try {
        return std::filesystem::path(path).filename().string();
    } catch (const std::exception& e) {
        std::cerr << "Error getting filename: " << e.what() << std::endl;
        return "";
    }
}

// Helper function to get filename without extension
std::string getFilenameWithoutExtension(const std::string& filename) {
    try {
        return std::filesystem::path(filename).stem().string();
    } catch (const std::exception& e) {
        std::cerr << "Error getting stem: " << e.what() << std::endl;
        return "";
    }
}

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
    assets.loadTexture("logo", "../assets/Image/logo.png");
    assets.loadTexture("player", "../assets/Image/player.png");
    assets.loadTexture("world", "../assets/Image/world.jpg");
}

DevModeScene::~DevModeScene() {
    std::cout << "Exiting Dev Mode Scene" << std::endl;
}

void DevModeScene::handleInput(SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    ImGuiIO& io = ImGui::GetIO();

    int rawMouseX, rawMouseY;
    Uint32 mouseButtonState = SDL_GetMouseState(&rawMouseX, &rawMouseY);

    bool mouseInViewport = (rawMouseX >= gameViewport.x && rawMouseX < (gameViewport.x + gameViewport.w) &&
                            rawMouseY >= gameViewport.y && rawMouseY < (gameViewport.y + gameViewport.h));

    float viewportMouseX = static_cast<float>(rawMouseX - gameViewport.x);
    float viewportMouseY = static_cast<float>(rawMouseY - gameViewport.y);

    float worldMouseX = viewportMouseX + cameraX;
    float worldMouseY = viewportMouseY + cameraY;

    static bool isPanning = false;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE && mouseInViewport && !io.WantCaptureMouse) {
        isPanning = true;
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE) {
        if (isPanning) {
            isPanning = false;
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }
    if (event.type == SDL_MOUSEMOTION && isPanning) {
        cameraX -= event.motion.xrel;
        cameraY -= event.motion.yrel;
    }

    if (isPlaying || isPanning) {
        isDragging = false;
        isResizing = false;
        activeHandle = ResizeHandle::NONE;
        return;
    }

    if (!io.WantCaptureMouse && mouseInViewport) {
        switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    bool clickedOnExistingSelection = false;
                    if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                        activeHandle = getHandleAtPosition(worldMouseX, worldMouseY, transform);

                        if (activeHandle != ResizeHandle::NONE) {
                            isResizing = true;
                            isDragging = false;
                            dragStartMouseX = worldMouseX;
                            dragStartMouseY = worldMouseY;
                            dragStartEntityX = transform.x;
                            dragStartEntityY = transform.y;
                            dragStartWidth = transform.width;
                            dragStartHeight = transform.height;
                            clickedOnExistingSelection = true;
                        } else if (isMouseOverEntity(worldMouseX, worldMouseY, selectedEntity)) {
                            isDragging = true;
                            isResizing = false;
                            dragStartMouseX = worldMouseX;
                            dragStartMouseY = worldMouseY;
                            dragStartEntityX = transform.x;
                            dragStartEntityY = transform.y;
                            clickedOnExistingSelection = true;
                        }
                    }

                    if (!clickedOnExistingSelection) {
                        isDragging = false;
                        isResizing = false;
                        activeHandle = ResizeHandle::NONE;
                        Entity clickedEntity = NO_ENTITY_SELECTED;
                        const auto& activeEntities = entityManager->getActiveEntities();
                        for (auto it = activeEntities.rbegin(); it != activeEntities.rend(); ++it) {
                            Entity entity = *it;
                            if (isMouseOverEntity(worldMouseX, worldMouseY, entity)) {
                                clickedEntity = entity;
                                break;
                            }
                        }

                        if (selectedEntity != clickedEntity) {
                            selectedEntity = clickedEntity;
                            inspectorTextureIdBuffer[0] = '\0';
                        }

                        if (selectedEntity != NO_ENTITY_SELECTED && isMouseOverEntity(worldMouseX, worldMouseY, selectedEntity)) {
                            isDragging = true;
                            auto& newTransform = componentManager->getComponent<TransformComponent>(selectedEntity);
                            dragStartMouseX = worldMouseX;
                            dragStartMouseY = worldMouseY;
                            dragStartEntityX = newTransform.x;
                            dragStartEntityY = newTransform.y;
                        }
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    if (isDragging && snapToGrid && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                        transform.x = std::roundf(transform.x / gridSize) * gridSize;
                        transform.y = std::roundf(transform.y / gridSize) * gridSize;
                    }
                    if (isResizing && snapToGrid && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                        auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                        float snappedWidth = std::max(gridSize, std::roundf(transform.width / gridSize) * gridSize);
                        float snappedHeight = std::max(gridSize, std::roundf(transform.height / gridSize) * gridSize);
                        float snappedX = transform.x;
                        float snappedY = transform.y;

                        if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::BOTTOM_LEFT) {
                            snappedX = std::roundf((transform.x + transform.width - snappedWidth) / gridSize) * gridSize;
                        } else {
                            snappedX = std::roundf(transform.x / gridSize) * gridSize;
                        }
                        if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::TOP_RIGHT) {
                            snappedY = std::roundf((transform.y + transform.height - snappedHeight) / gridSize) * gridSize;
                        } else {
                            snappedY = std::roundf(transform.y / gridSize) * gridSize;
                        }

                        transform.x = snappedX;
                        transform.y = snappedY;
                        transform.width = snappedWidth;
                        transform.height = snappedHeight;
                    }
                    isDragging = false;
                    isResizing = false;
                    activeHandle = ResizeHandle::NONE;
                }
                break;

            case SDL_MOUSEMOTION:
                if (isDragging && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
                    float deltaX = worldMouseX - dragStartMouseX;
                    float deltaY = worldMouseY - dragStartMouseY;
                    auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
                    transform.x = dragStartEntityX + deltaX;
                    transform.y = dragStartEntityY + deltaY;

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

                    if (newWidth < HANDLE_SIZE) {
                        if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::BOTTOM_LEFT) {
                            newX = transform.x + transform.width - HANDLE_SIZE;
                        }
                        newWidth = HANDLE_SIZE;
                    }
                    if (newHeight < HANDLE_SIZE) {
                        if (activeHandle == ResizeHandle::TOP_LEFT || activeHandle == ResizeHandle::TOP_RIGHT) {
                            newY = transform.y + transform.height - HANDLE_SIZE;
                        }
                        newHeight = HANDLE_SIZE;
                    }

                    transform.x = newX;
                    transform.y = newY;
                    transform.width = newWidth;
                    transform.height = newHeight;
                }
                break;
        }
    } else {
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            isDragging = false;
            isResizing = false;
            activeHandle = ResizeHandle::NONE;
        }
        if ((isDragging || isResizing) && event.type == SDL_MOUSEMOTION && !mouseInViewport) {
            isDragging = false;
            isResizing = false;
            activeHandle = ResizeHandle::NONE;
        }
    }
}

void DevModeScene::update(float deltaTime) {
    if (isPlaying) {
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

    gameViewport = {
        static_cast<int>(hierarchyWidth),
        static_cast<int>(topToolbarHeight),
        static_cast<int>(displaySize.x - hierarchyWidth - inspectorWidth),
        static_cast<int>(displaySize.y - topToolbarHeight - bottomPanelHeight)
    };
    if (gameViewport.w < 0) gameViewport.w = 0;
    if (gameViewport.h < 0) gameViewport.h = 0;

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);

    if (gameViewport.w > 0 && gameViewport.h > 0) {
        SDL_RenderSetViewport(renderer, &gameViewport);

        if (!isPlaying && showGrid) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            float gridStartX = -fmodf(cameraX, gridSize);
            float gridStartY = -fmodf(cameraY, gridSize);
            for (float x = gridStartX; x < gameViewport.w; x += gridSize) {
                SDL_RenderDrawLine(renderer, static_cast<int>(x), 0, static_cast<int>(x), gameViewport.h);
            }
            for (float y = gridStartY; y < gameViewport.h; y += gridSize) {
                SDL_RenderDrawLine(renderer, 0, static_cast<int>(y), gameViewport.w, static_cast<int>(y));
            }
        }

        renderSystem->update(renderer, componentManager.get(), cameraX, cameraY);

        if (!isPlaying && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
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
        SDL_RenderSetViewport(renderer, NULL);
    } else {
        SDL_RenderSetViewport(renderer, NULL);
    }

    ImGui::SetCursorScreenPos(ImVec2(static_cast<float>(gameViewport.x), static_cast<float>(gameViewport.y)));
    ImGui::InvisibleButton("##GameViewportDropTarget", ImVec2(static_cast<float>(gameViewport.w), static_cast<float>(gameViewport.h)));

    if (ImGui::BeginDragDropTarget()) {
        std::cout << "[DEBUG] Drop target active in game viewport" << std::endl;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE_ID")) {
            std::cout << "[DEBUG] Payload received in drop target" << std::endl;
            IM_ASSERT(payload->DataSize > 0);
            const char* textureIdCStr = (const char*)payload->Data;
            std::string textureId(textureIdCStr);
            std::cout << "[DEBUG] Dropped textureId: " << textureId << std::endl;

            ImVec2 viewportMousePos = ImVec2(ImGui::GetMousePos().x - ImGui::GetItemRectMin().x, ImGui::GetMousePos().y - ImGui::GetItemRectMin().y);

            float dropWorldX = viewportMousePos.x + cameraX;
            float dropWorldY = viewportMousePos.y + cameraY;

            if (snapToGrid) {
                dropWorldX = std::roundf(dropWorldX / gridSize) * gridSize;
                dropWorldY = std::roundf(dropWorldY / gridSize) * gridSize;
            }

            Entity newEntity = entityManager->createEntity();
            std::cout << "[DEBUG] Created new entity: " << newEntity << std::endl;
            TransformComponent transform;
            transform.x = dropWorldX;
            transform.y = dropWorldY;
            SDL_Texture* tex = AssetManager::getInstance().getTexture(textureId);
            if (tex) {
                int w, h;
                SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
                transform.width = (float)w;
                transform.height = (float)h;
            } else {
                transform.width = 32.0f;
                transform.height = 32.0f;
            }
            componentManager->addComponent(newEntity, transform);
            componentManager->addComponent(newEntity, SpriteComponent{textureId});
            Signature entitySignature;
            entitySignature.set(componentManager->getComponentType<TransformComponent>());
            entitySignature.set(componentManager->getComponentType<SpriteComponent>());
            entityManager->setSignature(newEntity, entitySignature);
            systemManager->entitySignatureChanged(newEntity, entitySignature);

            selectedEntity = newEntity;
            inspectorTextureIdBuffer[0] = '\0';
        }
        ImGui::EndDragDropTarget();
    }

    const ImGuiWindowFlags fixedPanelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, topToolbarHeight), ImGuiCond_Always);
    ImGui::Begin("Toolbar", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::Button("Save")) saveScene(sceneFilePath);
    ImGui::SameLine(); if (ImGui::Button("Save As...")) {}
    ImGui::SameLine(); if (ImGui::Button("Load")) loadScene(sceneFilePath);
    ImGui::SameLine(); ImGui::PushItemWidth(120); ImGui::InputText("##Filename", sceneFilePath, sizeof(sceneFilePath)); ImGui::PopItemWidth();
    ImGui::SameLine(); if (ImGui::Button("Import...")) {}
    ImGui::SameLine(); if (ImGui::Button("Export...")) {}
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    if (isPlaying) { if (ImGui::Button("Stop")) { isPlaying = false; loadScene(sceneFilePath); } }
    else { if (ImGui::Button("Play")) { isPlaying = true; selectedEntity = NO_ENTITY_SELECTED; } }
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
        if (strlen(spawnTextureId) > 0 && AssetManager::getInstance().getTexture(textureIdStr)) {
            componentManager->addComponent(newEntity, SpriteComponent{textureIdStr});
            entitySignature.set(componentManager->getComponentType<SpriteComponent>());
        } else if (strlen(spawnTextureId) > 0) {
            std::cerr << "Spawn Warning: Texture ID '" << textureIdStr << "' not found. Sprite not added." << std::endl;
        }
        entityManager->setSignature(newEntity, entitySignature);
        systemManager->entitySignatureChanged(newEntity, entitySignature);
    }
    ImGui::End();

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
                    std::string newTextureId(inspectorTextureIdBuffer);
                    AssetManager& assets = AssetManager::getInstance();
                    if (assets.getTexture(newTextureId) || assets.loadTexture(newTextureId, newTextureId)) {
                        sprite.textureId = newTextureId;
                    } else {
                        std::cerr << "Inspector Error: Failed to find or load texture: '" << newTextureId << "'. Reverting." << std::endl;
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
                if (ImGui::Button("Browse...##SpriteTexture")) {
                    const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
                    const char* filePath = tinyfd_openFileDialog("Select Texture File", "", 6, filterPatterns, "Image Files", 0);
                    if (filePath != NULL) {
                        std::string selectedPath = filePath;
                        std::string filename = getFilenameFromPath(selectedPath);
                        if (!filename.empty()) {
                            std::string assetId = getFilenameWithoutExtension(filename);
                            std::filesystem::path destDir = std::filesystem::absolute("../assets/Image/");
                            std::filesystem::path destPath = destDir / filename;

                            try {
                                std::filesystem::create_directories(destDir);
                                std::filesystem::copy_file(selectedPath, destPath, std::filesystem::copy_options::overwrite_existing);
                                std::cout << "Asset copied to: " << destPath.string() << std::endl;

                                AssetManager& assets = AssetManager::getInstance();
                                if (assets.loadTexture(assetId, destPath.string())) {
                                    sprite.textureId = assetId;
                                    strncpy(inspectorTextureIdBuffer, assetId.c_str(), IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1);
                                    inspectorTextureIdBuffer[IM_ARRAYSIZE(inspectorTextureIdBuffer) - 1] = '\0';
                                    std::cout << "Texture loaded and assigned: " << assetId << std::endl;
                                } else {
                                    tinyfd_messageBox("Error", "Failed to load texture into AssetManager.", "ok", "error", 1);
                                }
                            } catch (const std::filesystem::filesystem_error& e) {
                                std::cerr << "Filesystem Error: " << e.what() << std::endl;
                                tinyfd_messageBox("Error", ("Failed to copy file: " + std::string(e.what())).c_str(), "ok", "error", 1);
                            }
                        } else {
                            tinyfd_messageBox("Error", "Could not extract filename.", "ok", "error", 1);
                        }
                    }
                }
            }
        } else ImGui::TextDisabled("No Sprite Component");
    } else ImGui::Text("No entity selected.");
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, displaySize.y - bottomPanelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, bottomPanelHeight), ImGuiCond_Always);
    ImGui::Begin("BottomPanel", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoScrollbar);
    if (ImGui::BeginTabBar("BottomTabs")) {
        if (ImGui::BeginTabItem("Project")) {
            if (ImGui::Button("Import Asset")) {
                const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
                const char* filePath = tinyfd_openFileDialog("Import Asset File", "", 6, filterPatterns, "Image Files", 0);
                if (filePath != NULL) {
                    std::string selectedPath = filePath;
                    std::string filename = getFilenameFromPath(selectedPath);
                    if (!filename.empty()) {
                        std::string assetId = getFilenameWithoutExtension(filename);
                        std::filesystem::path destDir = std::filesystem::absolute("../assets/Image/");
                        std::filesystem::path destPath = destDir / filename;
                        try {
                            std::filesystem::create_directories(destDir);
                            std::filesystem::copy_file(selectedPath, destPath, std::filesystem::copy_options::overwrite_existing);
                            std::cout << "Asset copied to: " << destPath.string() << std::endl;
                            AssetManager& assets = AssetManager::getInstance();
                            if (!assets.loadTexture(assetId, destPath.string())) {
                                tinyfd_messageBox("Error", "Failed to load imported texture.", "ok", "error", 1);
                            }
                        } catch (const std::filesystem::filesystem_error& e) {
                            std::cerr << "Filesystem Error: " << e.what() << std::endl;
                            tinyfd_messageBox("Error", ("Failed to copy file: " + std::string(e.what())).c_str(), "ok", "error", 1);
                        }
                    } else {
                        tinyfd_messageBox("Error", "Could not extract filename.", "ok", "error", 1);
                    }
                }
            }
            ImGui::Separator();
            ImGui::Text("Available Textures:");
            AssetManager& assets = AssetManager::getInstance();
            const auto& textures = assets.getAllTextures();
            ImGuiStyle& style = ImGui::GetStyle();
            float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            float itemWidth = 64.0f;
            float itemSpacing = style.ItemSpacing.x;
            int itemsPerRow = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + itemSpacing) / (itemWidth + itemSpacing)));
            int currentItem = 0;

            for (const auto& [id, texture] : textures) {
                std::cout << "[DEBUG] Texture in loop: " << id << std::endl;
                ImGui::PushID(id.c_str());
                ImGui::BeginGroup();
                ImGui::Image((ImTextureID)texture, ImVec2(itemWidth, itemWidth));
                ImGui::TextWrapped("%s", id.c_str());
                ImGui::EndGroup();

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    std::cout << "[DEBUG] Begin drag for texture: " << id << std::endl;
                    ImGui::SetDragDropPayload("ASSET_TEXTURE_ID", id.c_str(), id.length() + 1);
                    ImGui::Image((ImTextureID)texture, ImVec2(itemWidth, itemWidth));
                    ImGui::Text("%s", id.c_str());
                    ImGui::EndDragDropSource();
                }

                currentItem++;
                if (currentItem % itemsPerRow != 0) {
                    float lastItemX = ImGui::GetItemRectMax().x;
                    float nextItemX = lastItemX + style.ItemSpacing.x + itemWidth;
                    if (nextItemX < (ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x)) {
                        ImGui::SameLine();
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Console")) {
            ImGui::TextWrapped("Console output placeholder...");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, SDL_GL_GetCurrentContext());
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
        tinyfd_messageBox("Save Error", ("Could not open file: " + filepath).c_str(), "ok", "error", 1);
    }
}

void DevModeScene::loadScene(const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file " << filepath << " for reading!" << std::endl;
        tinyfd_messageBox("Load Error", ("Could not open file: " + filepath).c_str(), "ok", "error", 1);
        return;
    }

    nlohmann::json sceneJson;
    try {
        inFile >> sceneJson;
        inFile.close();
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Error: Failed to parse scene file " << filepath << ". " << e.what() << std::endl;
        inFile.close();
        tinyfd_messageBox("Load Error", ("Failed to parse scene file: " + filepath + "\n" + e.what()).c_str(), "ok", "error", 1);
        return;
    }

    std::cout << "Loading scene from " << filepath << "..." << std::endl;

    std::set<Entity> entitiesToDestroy = entityManager->getActiveEntities();
    for (Entity entity : entitiesToDestroy) {
        entityManager->destroyEntity(entity);
    }
    if (!entityManager->getActiveEntities().empty()) {
        std::cerr << "Warning: Not all entities were destroyed during scene load cleanup!" << std::endl;
    }
    selectedEntity = NO_ENTITY_SELECTED;
    inspectorTextureIdBuffer[0] = '\0';
    cameraX = 0.0f;
    cameraY = 0.0f;

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
                        std::string potentialPath = "../assets/Image/" + comp.textureId + ".png";
                        if (!assets.loadTexture(comp.textureId, potentialPath)) {
                            potentialPath = "../assets/Image/" + comp.textureId + ".jpg";
                            if (!assets.loadTexture(comp.textureId, potentialPath)) {
                                std::cerr << "LoadScene Warning: Failed to dynamically load texture '" << comp.textureId
                                          << "' needed by loaded entity " << newEntity << ". Check path/extension." << std::endl;
                            } else {
                                std::cout << "LoadScene: Successfully loaded texture '" << comp.textureId << "' dynamically (as .jpg)." << std::endl;
                            }
                        } else {
                            std::cout << "LoadScene: Successfully loaded texture '" << comp.textureId << "' dynamically (as .png)." << std::endl;
                        }
                    }
                    componentManager->addComponent(newEntity, comp);
                    entitySignature.set(componentManager->getComponentType<SpriteComponent>());
                } else {
                    std::cerr << "Warning: Unknown component type '" << componentType << "' encountered during loading." << std::endl;
                }
            } catch (nlohmann::json::exception& e) {
                std::cerr << "Error parsing component '" << componentType << "'. " << e.what() << std::endl;
            }
        }
        entityManager->setSignature(newEntity, entitySignature);
        systemManager->entitySignatureChanged(newEntity, entitySignature);
    }
    std::cout << "Scene loaded successfully." << std::endl;
}

bool DevModeScene::isMouseOverEntity(float worldMouseX, float worldMouseY, Entity entity) {
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

ResizeHandle DevModeScene::getHandleAtPosition(float worldMouseX, float worldMouseY, const TransformComponent& transform) {
    for (const auto& handlePair : getResizeHandles(transform)) {
        const SDL_Rect& rect = handlePair.second;
        if (worldMouseX >= rect.x && worldMouseX < (rect.x + rect.w) &&
            worldMouseY >= rect.y && worldMouseY < (rect.y + rect.h)) {
            return handlePair.first;
        }
    }
    return ResizeHandle::NONE;
}
