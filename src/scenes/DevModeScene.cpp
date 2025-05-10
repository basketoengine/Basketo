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
#include <algorithm>

#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/ScriptComponent.h"
#include "../ecs/components/VelocityComponent.h"
#include "../ecs/components/ColliderComponent.h"
#include "../ecs/systems/RenderSystem.h"
#include "../ecs/systems/ScriptSystem.h"
#include "../ecs/systems/MovementSystem.h"
#include "../AssetManager.h"
#include "../utils/FileUtils.h" 
#include "../utils/EditorHelpers.h" 
#include "DevModeInputHandler.h"
#include "DevModeSceneSerializer.h" 
#include "InspectorPanel.h" 
#include <stack> 

DevModeScene::DevModeScene(SDL_Renderer* ren, SDL_Window* win) 
    : renderer(ren), 
      window(win),
      assetManager(AssetManager::getInstance()) 
{
    std::cout << "Entering Dev Mode Scene" << std::endl;

    entityManager = std::make_unique<EntityManager>();
    componentManager = std::make_unique<ComponentManager>();
    systemManager = std::make_unique<SystemManager>();

    componentManager->registerComponent<TransformComponent>();
    componentManager->registerComponent<SpriteComponent>();
    componentManager->registerComponent<VelocityComponent>();
    componentManager->registerComponent<ScriptComponent>(); 
    componentManager->registerComponent<ColliderComponent>();

    renderSystem = systemManager->registerSystem<RenderSystem>();
    Signature renderSig;
    renderSig.set(componentManager->getComponentType<TransformComponent>());
    renderSig.set(componentManager->getComponentType<SpriteComponent>());
    systemManager->setSignature<RenderSystem>(renderSig);

    scriptSystem = systemManager->registerSystem<ScriptSystem>(entityManager.get(), componentManager.get());
    scriptSystem->setLoggingFunctions(
        [this](const std::string& msg) { this->addLogToConsole(msg); },
        [this](const std::string& errMsg) { this->addLogToConsole(errMsg); }
    );
    scriptSystem->init(); 

    movementSystem = systemManager->registerSystem<MovementSystem>();
    Signature moveSig;
    moveSig.set(componentManager->getComponentType<TransformComponent>());
    moveSig.set(componentManager->getComponentType<VelocityComponent>());
    systemManager->setSignature<MovementSystem>(moveSig);

    AssetManager& assets = AssetManager::getInstance();
    std::string texturePath = "../assets/Textures/";
    if (std::filesystem::exists(texturePath)) {
        for (const auto& entry : std::filesystem::directory_iterator(texturePath)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string id = entry.path().stem().string();
                if (!assets.loadTexture(id, path)) {
                    std::cerr << "DevModeScene Error: Failed to load texture: " << path << std::endl;
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
                    std::cerr << "DevModeScene Error: Failed to load sound: " << path << std::endl;
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
                        std::cerr << "DevModeScene Error: Failed to load font: " << path << std::endl;
                    }
                }
            }
        }
    }
}

DevModeScene::~DevModeScene() {
    std::cout << "Exiting Dev Mode Scene" << std::endl;
}

void DevModeScene::handleInput(SDL_Event& event) {
    handleDevModeInput(*this, event);
}

void DevModeScene::update(float deltaTime) {
    if (isPlaying) {
        if (scriptSystem) {
            scriptSystem->update(deltaTime);
        }
        if (movementSystem) {
            movementSystem->update(componentManager.get(), deltaTime);
        }
    }
}

void DevModeScene::addLogToConsole(const std::string& message) {
    consoleLogBuffer.push_back(message);
    // Optional: Limit buffer size
    // if (consoleLogBuffer.size() > 1000) { // Example limit
    //     consoleLogBuffer.erase(consoleLogBuffer.begin(), consoleLogBuffer.begin() + consoleLogBuffer.size() - 1000);
    // }
}

void DevModeScene::render() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    const float hierarchyWidth = displaySize.x * hierarchyWidthRatio;
    const float inspectorWidth = displaySize.x * inspectorWidthRatio;
    const float bottomPanelHeight = displaySize.y * bottomPanelHeightRatio;

    gameViewport = {
        static_cast<int>(hierarchyWidth),
        static_cast<int>(topToolbarHeight),
        static_cast<int>(displaySize.x - hierarchyWidth - inspectorWidth),
        static_cast<int>(displaySize.y - topToolbarHeight - bottomPanelHeight)
    };
    if (gameViewport.w < 0) gameViewport.w = 0;
    if (gameViewport.h < 0) gameViewport.h = 0;

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(gameViewport.x), static_cast<float>(gameViewport.y)), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(gameViewport.w), static_cast<float>(gameViewport.h)), ImGuiCond_Always);
    ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
    if (!ImGui::IsDragDropActive()) {
        viewportFlags |= ImGuiWindowFlags_NoInputs;
    }
    ImGui::Begin("GameViewport", nullptr, viewportFlags);
    if (ImGui::IsDragDropActive()) {
        ImGui::InvisibleButton("##GameViewportDropTarget", ImVec2(static_cast<float>(gameViewport.w), static_cast<float>(gameViewport.h)));
        if (ImGui::BeginDragDropTarget()) {
            std::cout << "[DEBUG] Drop target active in game viewport" << std::endl;
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE_ID")) {
                std::cout << "[DEBUG] Payload received in drop target" << std::endl;
                IM_ASSERT(payload->DataSize > 0);
                const char* textureIdCStr = (const char*)payload->Data;
                std::string textureId(textureIdCStr);
                std::cout << "[DEBUG] Dropped textureId: " << textureId << std::endl;
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 itemMin = ImGui::GetItemRectMin();
                ImVec2 localPos;
                localPos.x = mousePos.x - itemMin.x;
                localPos.y = mousePos.y - itemMin.y;
                float dropWorldX = localPos.x + cameraX;
                float dropWorldY = localPos.y + cameraY;
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
    }
    ImGui::End();
    SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);
    if (gameViewport.w > 0 && gameViewport.h > 0) {
        SDL_RenderSetViewport(renderer, &gameViewport);

        if (!isPlaying && showGrid) {
            SDL_SetRenderDrawColor(renderer, 70, 70, 80, 255); 

            float worldViewLeft = cameraX;
            float worldViewTop = cameraY;
            float worldViewRight = cameraX + gameViewport.w / cameraZoom;
            float worldViewBottom = cameraY + gameViewport.h / cameraZoom;

            float gridStartX = std::floor(worldViewLeft / gridSize) * gridSize;
            float gridStartY = std::floor(worldViewTop / gridSize) * gridSize;

            for (float x = gridStartX; x < worldViewRight; x += gridSize) {
                int screenX = static_cast<int>((x - cameraX) * cameraZoom);
                SDL_RenderDrawLine(renderer, screenX, 0, screenX, gameViewport.h);
            }
            for (float y = gridStartY; y < worldViewBottom; y += gridSize) {
                int screenY = static_cast<int>((y - cameraY) * cameraZoom);
                SDL_RenderDrawLine(renderer, 0, screenY, gameViewport.w, screenY);
            }
        }

        std::vector<Entity> entitiesToRender;
        if (entityManager) { 
            for (auto entity : entityManager->getActiveEntities()) {
                if (componentManager && componentManager->hasComponent<TransformComponent>(entity) && componentManager->hasComponent<SpriteComponent>(entity)) {
                    entitiesToRender.push_back(entity);
                }
            }
        }
        std::sort(entitiesToRender.begin(), entitiesToRender.end(),
            [&](Entity a, Entity b) {
                if (!componentManager || !componentManager->hasComponent<TransformComponent>(a) || !componentManager->hasComponent<TransformComponent>(b)) {
                    return false; 
                }
                auto& transformA = componentManager->getComponent<TransformComponent>(a);
                auto& transformB = componentManager->getComponent<TransformComponent>(b);
                return transformA.z_index < transformB.z_index;
            }
        );
        
        for (auto entity : entitiesToRender) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            auto& sprite = componentManager->getComponent<SpriteComponent>(entity);
            SDL_Texture* texture = AssetManager::getInstance().getTexture(sprite.textureId);
            if (!texture) continue;
            SDL_Rect destRect;
            destRect.x = (int)((transform.x - cameraX) * cameraZoom);
            destRect.y = (int)((transform.y - cameraY) * cameraZoom);
            destRect.w = (int)(transform.width * cameraZoom);
            destRect.h = (int)(transform.height * cameraZoom);
            SDL_Rect* srcRectPtr = sprite.useSrcRect ? &sprite.srcRect : nullptr;
            SDL_Point center = { (int)(transform.width * cameraZoom / 2), (int)(transform.height * cameraZoom / 2) };
            SDL_RenderCopyEx(renderer, texture, srcRectPtr, &destRect, transform.rotation, &center, SDL_FLIP_NONE);
        }

        if (!isPlaying) { 
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 150); 
            if (entityManager && componentManager) { 
                for (auto entity : entityManager->getActiveEntities()) {
                    if (componentManager->hasComponent<TransformComponent>(entity) && componentManager->hasComponent<ColliderComponent>(entity)) {
                        auto& transform = componentManager->getComponent<TransformComponent>(entity);
                        auto& collider = componentManager->getComponent<ColliderComponent>(entity);
                        if (!collider.vertices.empty()) {
                            for (size_t i = 0; i < collider.vertices.size(); ++i) {
                                size_t j = (i + 1) % collider.vertices.size();
                                float x1 = (transform.x + collider.offsetX + collider.vertices[i].x - cameraX) * cameraZoom;
                                float y1 = (transform.y + collider.offsetY + collider.vertices[i].y - cameraY) * cameraZoom;
                                float x2 = (transform.x + collider.offsetX + collider.vertices[j].x - cameraX) * cameraZoom;
                                float y2 = (transform.y + collider.offsetY + collider.vertices[j].y - cameraY) * cameraZoom;
                                SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
                            }
                        } else {
                            SDL_Rect colliderRect = {
                                (int)(((transform.x + collider.offsetX) - cameraX) * cameraZoom),
                                (int)(((transform.y + collider.offsetY) - cameraY) * cameraZoom),
                                (int)(collider.width * cameraZoom),
                                (int)(collider.height * cameraZoom)
                            };
                            SDL_RenderDrawRect(renderer, &colliderRect);
                        }
                    }
                }
            }
        }


        if (!isPlaying && selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
            auto& transform = componentManager->getComponent<TransformComponent>(selectedEntity);
            SDL_Rect selectionRect = {
                (int)((transform.x - cameraX) * cameraZoom),
                (int)((transform.y - cameraY) * cameraZoom),
                (int)(transform.width * cameraZoom),
                (int)(transform.height * cameraZoom)
            };
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(renderer, &selectionRect);
            SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
            for (const auto& handlePair : getResizeHandles(transform)) {
                SDL_Rect handleRect = handlePair.second;
                handleRect.x = (int)((handleRect.x - cameraX) * cameraZoom);
                handleRect.y = (int)((handleRect.y - cameraY) * cameraZoom);
                handleRect.w = (int)(handleRect.w * cameraZoom);
                handleRect.h = (int)(handleRect.h * cameraZoom);
                SDL_RenderFillRect(renderer, &handleRect);
            }
        }
        SDL_RenderSetViewport(renderer, NULL);
    } else {
        SDL_RenderSetViewport(renderer, NULL);
    }

    const ImGuiWindowFlags fixedPanelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, topToolbarHeight), ImGuiCond_Always);
    ImGui::Begin("Toolbar", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::Button("Save")) saveDevModeScene(*this, sceneFilePath); 
    ImGui::SameLine(); if (ImGui::Button("Save As...")) { /* TODO */ }
    ImGui::SameLine(); if (ImGui::Button("Load")) loadDevModeScene(*this, sceneFilePath); 
    ImGui::SameLine(); ImGui::PushItemWidth(120); ImGui::InputText("##Filename", sceneFilePath, sizeof(sceneFilePath)); ImGui::PopItemWidth();
    ImGui::SameLine(); if (ImGui::Button("Import...")) {}
    ImGui::SameLine(); if (ImGui::Button("Export...")) {}
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    if (isPlaying) {
        if (ImGui::Button("Stop")) {
            isPlaying = false;
            loadDevModeScene(*this, sceneFilePath);
        }
    } else {
        if (ImGui::Button("Play")) {
            isPlaying = true;
            selectedEntity = NO_ENTITY_SELECTED;
            for (auto entity : entityManager->getActiveEntities()) {
                if (componentManager->hasComponent<ScriptComponent>(entity)) {
                    auto& scriptComp = componentManager->getComponent<ScriptComponent>(entity);
                    if (!scriptComp.scriptPath.empty()) {
                        scriptSystem->loadScript(entity, scriptComp.scriptPath);
                    }
                }
            }
        }
    }
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
            inspectorScriptPathBuffer[0] = '\0'; 
        }
    }
    if (ImGui::Button("Deselect")) {
        selectedEntity = NO_ENTITY_SELECTED;
        inspectorTextureIdBuffer[0] = '\0';
        inspectorScriptPathBuffer[0] = '\0'; 
    }
    ImGui::End();

    EditorUI::renderInspectorPanel(*this, io); 

    ImGui::SetNextWindowPos(ImVec2(0, displaySize.y - bottomPanelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(displaySize.x, bottomPanelHeight), ImGuiCond_Always);
    ImGui::Begin("BottomPanel", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoScrollbar);
    if (ImGui::BeginTabBar("BottomTabs")) {
        if (ImGui::BeginTabItem("Project")) {
            if (ImGui::Button("Import Asset")) {
                const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga", "*.mp3", "*.wav", "*.ogg", "*.flac", "*.ttf", "*.otf", "*.lua", "*.json" }; // Added more patterns
                const char* filePath = tinyfd_openFileDialog("Import Asset File", "", 14, filterPatterns, "Asset Files", 0); // Adjusted count
                if (filePath != NULL) {
                    std::string selectedPath = filePath;
                    std::filesystem::path p = selectedPath;
                    std::string filename = p.filename().string();
                    if (!filename.empty()) {
                        std::string assetId = p.stem().string();
                        std::filesystem::path destDir;
                        std::string ext = p.extension().string();
                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif" || ext == ".tga") {
                            destDir = std::filesystem::absolute("../assets/Textures/");
                        } else if (ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac") {
                            destDir = std::filesystem::absolute("../assets/Audio/");
                        } else if (ext == ".ttf" || ext == ".otf") {
                            destDir = std::filesystem::absolute("../assets/Fonts/");
                        } else if (ext == ".lua") {
                            destDir = std::filesystem::absolute("../assets/Scripts/");
                        } else if (ext == ".json") { 
                            destDir = std::filesystem::absolute("../assets/Scenes/");
                        }
                         else {
                            destDir = std::filesystem::absolute("../assets/"); 
                        }
                        std::filesystem::path destPath = destDir / filename;
                        try {
                            std::filesystem::create_directories(destDir);
                            std::filesystem::copy_file(selectedPath, destPath, std::filesystem::copy_options::overwrite_existing);
                            std::cout << "Asset copied to: " << destPath.string() << std::endl;
                            AssetManager& assets = AssetManager::getInstance();
                            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif" || ext == ".tga") {
                                if (!assets.loadTexture(assetId, destPath.string())) {
                                    tinyfd_messageBox("Error", "Failed to load imported texture.", "ok", "error", 1);
                                }
                            } else if (ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac") {
                                if (!assets.loadSound(assetId, destPath.string())) {
                                    tinyfd_messageBox("Error", "Failed to load imported sound.", "ok", "error", 1);
                                }
                            } else if (ext == ".ttf" || ext == ".otf") {
                                 if (!assets.loadFont(assetId + "_16", destPath.string(), 16)) { 
                                    tinyfd_messageBox("Error", "Failed to load imported font.", "ok", "error", 1);
                                }
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

            std::function<void(const std::filesystem::path&)> displayAssetTree;
            displayAssetTree = [&](const std::filesystem::path& currentPath) {
                namespace fs = std::filesystem;
                AssetManager& assets = AssetManager::getInstance();
                ImGuiStyle& style = ImGui::GetStyle();
                float itemWidth = 64.0f; 

                std::vector<fs::directory_entry> entries;
                try {
                    for (const auto& entry : fs::directory_iterator(currentPath)) {
                        entries.push_back(entry);
                    }
                } catch (const fs::filesystem_error& e) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error accessing %s: %s", currentPath.string().c_str(), e.what());
                    return;
                }

                std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    if (a.is_directory() && !b.is_directory()) return true;
                    if (!a.is_directory() && b.is_directory()) return false;
                    return a.path().filename().string() < b.path().filename().string();
                });

                for (const auto& entry : entries) {
                    const auto& path = entry.path();
                    std::string filename = path.filename().string();
                    std::string assetId = path.stem().string();

                    if (entry.is_directory()) {
                        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                        bool node_open = ImGui::TreeNodeEx(filename.c_str(), node_flags, "[F] %s", filename.c_str());
                        if (node_open) {
                            displayAssetTree(path);
                            ImGui::TreePop();
                        }
                    } else if (entry.is_regular_file()) {
                        std::string ext = path.extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif" || ext == ".tga") {
                            SDL_Texture* texture = assets.getTexture(assetId);
                            ImGui::BeginGroup();
                            if (texture) {
                                ImGui::Image((ImTextureID)texture, ImVec2(itemWidth, itemWidth));
                            } else {
                                ImGui::Dummy(ImVec2(itemWidth, itemWidth)); 
                                ImGui::TextWrapped("(No Preview)");
                            }
                            ImGui::TextWrapped("%s", filename.c_str());
                            ImGui::EndGroup();

                            if (texture && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                ImGui::SetDragDropPayload("ASSET_TEXTURE_ID", assetId.c_str(), assetId.length() + 1);
                                ImGui::Image((ImTextureID)texture, ImVec2(itemWidth, itemWidth));
                                ImGui::Text("%s", filename.c_str());
                                ImGui::EndDragDropSource();
                            }
                        } else if (ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac") {
                            ImGui::BulletText("[A] %s", filename.c_str());
                        } else if (ext == ".ttf" || ext == ".otf") {
                            ImGui::BulletText("[T] %s", filename.c_str());
                        } else if (ext == ".lua") {
                            ImGui::BulletText("[S] %s", filename.c_str());
                        } else if (ext == ".json") { 
                            ImGui::BulletText("[J] %s", filename.c_str());
                            if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0)) {
                                std::string sceneToLoadPath = fs::relative(path, fs::current_path() / "../").string();
                                std::replace(sceneToLoadPath.begin(), sceneToLoadPath.end(), '\\', '/');
                                if (loadDevModeScene(*this, sceneToLoadPath)) {
                                    strncpy(sceneFilePath, sceneToLoadPath.c_str(), sizeof(sceneFilePath) - 1);
                                    sceneFilePath[sizeof(sceneFilePath) - 1] = '\0';
                                    std::cout << "Loaded scene: " << sceneToLoadPath << std::endl;
                                } else {
                                    std::cerr << "Failed to load scene: " << sceneToLoadPath << std::endl;
                                }
                            }
                        }
                         else {
                            ImGui::BulletText("[?] %s", filename.c_str());
                        }
                        float currentX = ImGui::GetCursorPosX() + ImGui::GetItemRectSize().x + style.ItemSpacing.x;
                        float windowWidth = ImGui::GetWindowContentRegionMax().x;
                        if (currentX < windowWidth) {
                            // Check if the next item would fit, otherwise wrap. This logic is tricky with varying item widths.
                            // For simplicity, this example doesn't implement complex wrapping for non-texture items.
                            // ImGui::SameLine(); // Only if you want items side-by-side
                        }
                    }
                }
            };

            std::filesystem::path assetsRoot = "../assets";
            if (std::filesystem::exists(assetsRoot) && std::filesystem::is_directory(assetsRoot)) {
                displayAssetTree(assetsRoot);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Assets directory not found at: %s", std::filesystem::absolute(assetsRoot).string().c_str());
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Console")) {
            if (ImGui::Button("Clear")) {
                consoleLogBuffer.clear();
            }
            ImGui::SameLine();

            ImGui::Separator();
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& log : consoleLogBuffer) {
                ImGui::TextUnformatted(log.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f); 
            }
            ImGui::EndChild();
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
