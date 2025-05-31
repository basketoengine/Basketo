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
#include <SDL2/SDL_mixer.h>

#include "./DevModeSceneSerializer.h"
#include "../ecs/components/RigidbodyComponent.h" 
#include "../ecs/components/NameComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/ScriptComponent.h" 
#include "../ecs/components/ColliderComponent.h"
#include "../ecs/components/AnimationComponent.h"
#include "../ecs/components/AudioComponent.h"
#include "../AssetManager.h" 
#include "../ecs/systems/AudioSystem.h"
#include "../ecs/systems/CameraSystem.h"
#include "../ecs/systems/CollisionSystem.h" 
#include "../ai/AIPromptProcessor.h" 
#include "../ecs/systems/PhysicsSystem.h" 


DevModeScene::DevModeScene(SDL_Renderer* ren, SDL_Window* win)
    : renderer(ren),
      window(win),
      assetManager(AssetManager::getInstance()),
      entityManager(std::make_unique<EntityManager>()),
      componentManager(std::make_unique<ComponentManager>()),
      systemManager(std::make_unique<SystemManager>()),
      m_devModeInputHandler(*this),
      m_aiPromptProcessor(std::make_unique<AIPromptProcessor>(
          entityManager.get(),
          componentManager.get(),
          systemManager.get(),
          &assetManager,
          [this](const std::string& name) { return this->findEntityByName(name); }
      )) {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   

    assetManager.init(renderer);

    componentManager->registerComponent<TransformComponent>();
    componentManager->registerComponent<SpriteComponent>();
    componentManager->registerComponent<VelocityComponent>();
    componentManager->registerComponent<ScriptComponent>();
    componentManager->registerComponent<ColliderComponent>(); 
    componentManager->registerComponent<NameComponent>();
    componentManager->registerComponent<AnimationComponent>();
    componentManager->registerComponent<AudioComponent>(); 
    componentManager->registerComponent<RigidbodyComponent>(); 
    componentManager->registerComponent<CameraComponent>(); 
    loadDevModeScene(*this, sceneFilePath);

    renderSystem = systemManager->registerSystem<RenderSystem>();
    movementSystem = systemManager->registerSystem<MovementSystem>();
    scriptSystem = systemManager->registerSystem<ScriptSystem>(entityManager.get(), componentManager.get());
    animationSystem = systemManager->registerSystem<AnimationSystem>();
    audioSystem = systemManager->registerSystem<AudioSystem>();
    cameraSystem = systemManager->registerSystem<CameraSystem>(componentManager.get(), entityManager.get(), renderer);
    collisionSystem = systemManager->registerSystem<CollisionSystem>();
    physicsSystem = systemManager->registerSystem<PhysicsSystem>(); 

    scriptSystem->setLoggingFunctions(
        [this](const std::string& msg) { this->addLogToConsole(msg); },
        [this](const std::string& errMsg) { this->addLogToConsole(errMsg); }
    );
    scriptSystem->init(); 

    m_aiPromptProcessor = std::make_unique<AIPromptProcessor>(
        entityManager.get(),
        componentManager.get(),
        systemManager.get(),
        &assetManager, 
        [this](const std::string& name) { return this->findEntityByName(name); }
    );

    Signature renderSig;
    renderSig.set(componentManager->getComponentType<TransformComponent>());
    renderSig.set(componentManager->getComponentType<SpriteComponent>());
    systemManager->setSignature<RenderSystem>(renderSig);

    Signature moveSig;
    moveSig.set(componentManager->getComponentType<TransformComponent>());
    moveSig.set(componentManager->getComponentType<VelocityComponent>());
    systemManager->setSignature<MovementSystem>(moveSig);

    Signature animSig;
    animSig.set(componentManager->getComponentType<SpriteComponent>());
    animSig.set(componentManager->getComponentType<AnimationComponent>());
    systemManager->setSignature<AnimationSystem>(animSig);

    Signature audioSig;
    audioSig.set(componentManager->getComponentType<AudioComponent>());
    systemManager->setSignature<AudioSystem>(audioSig);

    Signature scriptSig;
    scriptSig.set(componentManager->getComponentType<ScriptComponent>());
    systemManager->setSignature<ScriptSystem>(scriptSig);

    Signature cameraSig;
    cameraSig.set(componentManager->getComponentType<TransformComponent>());
    cameraSig.set(componentManager->getComponentType<CameraComponent>());
    systemManager->setSignature<CameraSystem>(cameraSig);

    Signature collisionSig;
    collisionSig.set(componentManager->getComponentType<TransformComponent>());
    collisionSig.set(componentManager->getComponentType<ColliderComponent>());
    systemManager->setSignature<CollisionSystem>(collisionSig);

    Signature physicsSig; 
    physicsSig.set(componentManager->getComponentType<TransformComponent>());
    physicsSig.set(componentManager->getComponentType<VelocityComponent>());
    physicsSig.set(componentManager->getComponentType<RigidbodyComponent>());
    systemManager->setSignature<PhysicsSystem>(physicsSig);

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

    std::string texturesRoot = "../assets/Textures/";
    namespace fs = std::filesystem;
    if (fs::exists(texturesRoot)) {
        for (const auto& entry : fs::recursive_directory_iterator(texturesRoot)) {
            if (entry.is_regular_file()) {
                std::string fullPath = entry.path().string();
                std::string relPath = fs::relative(entry.path(), texturesRoot).string();
                AssetManager::getInstance().loadTexture(relPath, fullPath);
            }
        }
    }

}

DevModeScene::~DevModeScene() {
    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    std::cout << "Exiting Dev Mode Scene" << std::endl;
}

void DevModeScene::handleInput(SDL_Event& event) {
    handleDevModeInput(*this, event);
}

void DevModeScene::update(float deltaTime) {
    if (!isPlaying) {
        if (m_aiPromptProcessor) {
            m_aiPromptProcessor->PollAndProcessPendingCommands();
        }
        return; 
    }

    // Recommended new order:
    // 1. Scripts (can influence physics inputs)
    if (scriptSystem) {
        scriptSystem->update(deltaTime);
    }

    // 2. Physics (calculates forces, updates velocities)
    if (physicsSystem) { 
        physicsSystem->update(componentManager.get(), deltaTime);
    }

    // 3. Movement (applies velocities to transforms)
    if (movementSystem) {
        movementSystem->update(componentManager.get(), deltaTime); 
    }

    // 4. Collision (detects and resolves collisions, can alter transforms and velocities)
    if (collisionSystem) {
        collisionSystem->update(componentManager.get(), deltaTime);
    }

    // 5. Animation
    if (animationSystem) {
        animationSystem->update(deltaTime, *entityManager, *componentManager); 
    }

    // 6. Audio
    if (audioSystem) {
        audioSystem->update(deltaTime, *entityManager, *componentManager);
    }

    // 7. Camera (often depends on final positions of entities)
    if (cameraSystem) {
        cameraSystem->update(gameViewport, cameraZoom);
    }
}

void DevModeScene::addLogToConsole(const std::string& message) {
    consoleLogBuffer.push_back(message);

}

void DevModeScene::render() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    float currentRenderCameraX = this->cameraX;
    float currentRenderCameraY = this->cameraY;  
    float currentRenderZoom = this->cameraZoom; 
    
    if (isPlaying && cameraSystem) {
        SDL_Rect activeGameCameraWorldView;
        float gameCamZoom = 1.0f;
        cameraSystem->update(activeGameCameraWorldView, gameCamZoom);
        Entity activeCamEntity = cameraSystem->getActiveCameraEntity();

        if (activeCamEntity != NO_ENTITY) {
            // If a game camera is active and we are playing, use its properties for rendering.
            // The CameraSystem::update gives us the top-left of the world view (activeGameCameraWorldView.x, .y)
            // and the zoom level (gameCamZoom).
            // The width/height from activeGameCameraWorldView (activeGameCameraWorldView.w, .h) 
            // represent the amount of world space visible through that camera.
            currentRenderCameraX = static_cast<float>(activeGameCameraWorldView.x);
            currentRenderCameraY = static_cast<float>(activeGameCameraWorldView.y);
            currentRenderZoom = gameCamZoom;
        }
    }

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
            std::string assetType;
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

            float worldViewLeft = currentRenderCameraX; 
            float worldViewTop = currentRenderCameraY;  
            // Calculate how much of the world is visible in the gameViewport with the current zoom
            float visibleWorldWidthInViewport = gameViewport.w / currentRenderZoom;
            float visibleWorldHeightInViewport = gameViewport.h / currentRenderZoom;
            float worldViewRight = currentRenderCameraX + visibleWorldWidthInViewport; 
            float worldViewBottom = currentRenderCameraY + visibleWorldHeightInViewport; 

            float gridStartX = std::floor(worldViewLeft / gridSize) * gridSize;
            float gridStartY = std::floor(worldViewTop / gridSize) * gridSize;

            for (float x = gridStartX; x < worldViewRight; x += gridSize) {
                int screenX = static_cast<int>((x - currentRenderCameraX) * currentRenderZoom); 
                SDL_RenderDrawLine(renderer, screenX, 0, screenX, gameViewport.h);
            }
            for (float y = gridStartY; y < worldViewBottom; y += gridSize) {
                int screenY = static_cast<int>((y - currentRenderCameraY) * currentRenderZoom); 
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
            destRect.x = (int)((transform.x - currentRenderCameraX) * currentRenderZoom); 
            destRect.y = (int)((transform.y - currentRenderCameraY) * currentRenderZoom); 
            destRect.w = (int)(transform.width * currentRenderZoom);                       
            destRect.h = (int)(transform.height * currentRenderZoom);                      
            SDL_Rect* srcRectPtr = sprite.useSrcRect ? &sprite.srcRect : nullptr;
            SDL_Point center = { (int)(transform.width * currentRenderZoom / 2), (int)(transform.height * currentRenderZoom / 2) }; 
            SDL_RenderCopyEx(renderer, texture, srcRectPtr, &destRect, transform.rotation, &center, sprite.flip);
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
                                float x1 = (transform.x + collider.offsetX + collider.vertices[i].x - currentRenderCameraX) * currentRenderZoom; 
                                float y1 = (transform.y + collider.offsetY + collider.vertices[i].y - currentRenderCameraY) * currentRenderZoom; 
                                float x2 = (transform.x + collider.offsetX + collider.vertices[j].x - currentRenderCameraX) * currentRenderZoom; 
                                float y2 = (transform.y + collider.offsetY + collider.vertices[j].y - currentRenderCameraY) * currentRenderZoom; 
                                SDL_RenderDrawLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
                            }
                        } else {
                            SDL_Rect colliderRect = {
                                (int)(((transform.x + collider.offsetX) - currentRenderCameraX) * currentRenderZoom), 
                                (int)(((transform.y + collider.offsetY) - currentRenderCameraY) * currentRenderZoom), 
                                (int)(collider.width * currentRenderZoom),                                         
                                (int)(collider.height * currentRenderZoom)                                        
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
                (int)((transform.x - currentRenderCameraX) * currentRenderZoom), 
                (int)((transform.y - currentRenderCameraY) * currentRenderZoom), 
                (int)(transform.width * currentRenderZoom),                     
                (int)(transform.height * currentRenderZoom)                    
            };
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(renderer, &selectionRect);
            SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
            for (const auto& handlePair : getResizeHandles(transform)) {
                SDL_Rect handleRect = handlePair.second;

                handleRect.x = (int)((handleRect.x - currentRenderCameraX) * currentRenderZoom); 
                handleRect.y = (int)((handleRect.y - currentRenderCameraY) * currentRenderZoom); 
                handleRect.w = (int)(handleRect.w * currentRenderZoom);                         
                handleRect.h = (int)(handleRect.h * currentRenderZoom);                         
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
            Mix_HaltMusic();
            Mix_HaltChannel(-1);
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

    if (ImGui::CollapsingHeader("Spawn Entity", ImGuiTreeNodeFlags_None)) {
        ImGui::Text("Spawn Pos:"); ImGui::SameLine(); ImGui::PushItemWidth(40); ImGui::InputFloat("X", &spawnPosX, 0, 0, "%.0f"); ImGui::SameLine(); ImGui::InputFloat("Y", &spawnPosY, 0, 0, "%.0f"); ImGui::PopItemWidth();
        ImGui::SameLine(); ImGui::Text("Size:"); ImGui::SameLine(); ImGui::PushItemWidth(40); ImGui::InputFloat("W", &spawnSizeW, 0, 0, "%.0f"); ImGui::SameLine(); ImGui::InputFloat("H", &spawnSizeH, 0, 0, "%.0f"); ImGui::PopItemWidth();
        ImGui::SameLine(); ImGui::Text("TexID:"); ImGui::SameLine(); ImGui::PushItemWidth(60); ImGui::InputText("##SpawnTexID", spawnTextureId, IM_ARRAYSIZE(spawnTextureId)); ImGui::PopItemWidth(); ImGui::SameLine();
        if (ImGui::Button("Spawn##Button")) {
            Entity newEntity = entityManager->createEntity();
            // Spawn relative to the center of the current camera's view
            float viewCenterX = currentRenderCameraX + (gameViewport.w / (2.0f * currentRenderZoom));
            float viewCenterY = currentRenderCameraY + (gameViewport.h / (2.0f * currentRenderZoom));
            TransformComponent transform; transform.x = viewCenterX + spawnPosX; transform.y = viewCenterY + spawnPosY; transform.width = spawnSizeW > 0 ? spawnSizeW : 32; transform.height = spawnSizeH > 0 ? spawnSizeH : 32;
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
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(0, topToolbarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(hierarchyWidth, displaySize.y - topToolbarHeight - bottomPanelHeight), ImGuiCond_Always);
    if (ImGui::Begin("Hierarchy", nullptr, fixedPanelFlags)) {
        if (ImGui::BeginTabBar("LeftTabs")) {
            if (ImGui::BeginTabItem("Entities")) {
                ImGui::Text("Entities:");
                ImGui::Separator();
                const auto& activeEntities = entityManager->getActiveEntities();
                for (Entity entity : activeEntities) {
                    std::string label = "Entity " + std::to_string(entity);
                    if (componentManager->hasComponent<NameComponent>(entity)) {
                        label = componentManager->getComponent<NameComponent>(entity).name;
                    }
                    if (componentManager->hasComponent<TransformComponent>(entity)) {
                        label += " (Z: " + std::to_string(componentManager->getComponent<TransformComponent>(entity).z_index) + ")";
                    }
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
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("AI Prompt")) {
                if (m_aiPromptProcessor) { // Added null check
                    m_aiPromptProcessor->renderAIPromptUI();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    EditorUI::renderInspectorPanel(*this, io); 

    // Asset Preview Panel (Bottom Right)  NEEEEED TO BE FIXED
    ImGui::SetNextWindowPos(ImVec2(hierarchyWidth + gameViewport.w, displaySize.y - bottomPanelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(inspectorWidth, bottomPanelHeight), ImGuiCond_Always);
    ImGui::Begin("AssetPreview", nullptr, fixedPanelFlags | ImGuiWindowFlags_NoScrollbar);
    if (!selectedAssetPathForPreview.empty()) {
        std::filesystem::path assetPathObj(selectedAssetPathForPreview);
        // std::string assetId = assetPathObj.stem().string(); // Old problematic way
        std::string correctAssetId;

        std::string assetTypeRootPathStr;
        if (selectedAssetTypeForPreview == "texture") {
            assetTypeRootPathStr = "../assets/Textures/";
        } else if (selectedAssetTypeForPreview == "audio") {
            assetTypeRootPathStr = "../assets/Audio/";
        } else if (selectedAssetTypeForPreview == "animation") {
            assetTypeRootPathStr = "../assets/Animations/";
        } else if (selectedAssetTypeForPreview == "font") {
            assetTypeRootPathStr = "../assets/Fonts/";
        } else if (selectedAssetTypeForPreview == "script") {
            assetTypeRootPathStr = "../assets/Scripts/";
        }

        if (!assetTypeRootPathStr.empty()) {
            try {
                std::filesystem::path typeRootPath = std::filesystem::absolute(assetTypeRootPathStr);
                correctAssetId = std::filesystem::relative(assetPathObj, typeRootPath).string();
                // Normalize path separators to '/'
                std::replace(correctAssetId.begin(), correctAssetId.end(), '\\', '/');
            } catch (const std::filesystem::filesystem_error& e) {
                // Fallback if relative path fails (e.g. different drives on Windows, though less likely here)
                correctAssetId = assetPathObj.filename().string();
                addLogToConsole("Filesystem error calculating relative path for preview: " + std::string(e.what()) + ". Falling back to filename.");
            }
        } else {
            correctAssetId = assetPathObj.filename().string(); // Fallback if type root is not defined
        }

        std::string extension = assetPathObj.extension().string();
        std::string extLower = extension;
        std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);

        ImGui::TextWrapped("Preview: %s", assetPathObj.filename().string().c_str());
        // ImGui::TextWrapped("ID: %s", correctAssetId.c_str()); // Optional: for debugging the ID

        if (selectedAssetTypeForPreview == "texture" && (extLower == ".png" || extLower == ".jpg" || extLower == ".jpeg")) {
            SDL_Texture* texture = assetManager.getTexture(correctAssetId);
            if (texture) {
                int w, h;
                SDL_QueryTexture(texture, NULL, NULL, &w, &h);
                float aspectRatio = static_cast<float>(w) / h;
                float previewWidth = ImGui::GetContentRegionAvail().x;
                float previewHeight = previewWidth / aspectRatio;

                if (previewHeight > ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2) { // Adjusted for text lines
                    previewHeight = ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2;
                    previewWidth = previewHeight * aspectRatio;
                }
                 if (previewWidth > ImGui::GetContentRegionAvail().x) {
                    previewWidth = ImGui::GetContentRegionAvail().x;
                    previewHeight = previewWidth / aspectRatio;
                }
                ImGui::Image((ImTextureID)(uintptr_t)texture, ImVec2(previewWidth, previewHeight));
            } else {
                ImGui::TextWrapped("Texture not found or failed to load.");
                ImGui::TextWrapped("Attempted ID: %s", correctAssetId.c_str());
                // ImGui::TextWrapped("Full path: %s", selectedAssetPathForPreview.c_str()); // Already shown as "Preview:"
            }
        } else if (selectedAssetTypeForPreview == "audio" && (extLower == ".mp3" || extLower == ".wav" || extLower == ".ogg")) {
            ImGui::TextWrapped("Audio File: %s", assetPathObj.filename().string().c_str());
            if (ImGui::Button("Play")) {
                Mix_Chunk* sound = assetManager.getSound(correctAssetId);
                if (sound) {
                    Mix_PlayChannel(-1, sound, 0);
                } else {
                    addLogToConsole("Error: Sound not found with ID: " + correctAssetId);
                }
            }
        } else if (selectedAssetTypeForPreview == "animation" && extLower == ".json") {
            ImGui::TextWrapped("Animation File: %s", assetPathObj.filename().string().c_str());
            // Add animation preview logic if needed, using correctAssetId
        } else if (selectedAssetTypeForPreview == "font" && (extLower == ".ttf" || extLower == ".otf")) {
            ImGui::TextWrapped("Font File: %s", assetPathObj.filename().string().c_str());
            // Font preview might require specific ID format (e.g., "fontname_size")
            // For now, just displaying filename.
        } else if (selectedAssetTypeForPreview == "script" && extLower == ".lua") {
            ImGui::TextWrapped("Script File: %s", assetPathObj.filename().string().c_str());
            // Script preview logic
        }
        else {
            ImGui::TextWrapped("Unsupported file type for preview or unknown asset type for ID generation.");
        }
    } else {
        ImGui::Text("Select an asset to preview.");
    }
    ImGui::End();


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
                        std::string assetTypeToImport;

                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif" || ext == ".tga") {
                            destDir = std::filesystem::absolute("../assets/Textures/");
                            assetTypeToImport = "texture";
                        } else if (ext == ".mp3" || ext == ".wav" || ext == ".ogg" || ext == ".flac") {
                            destDir = std::filesystem::absolute("../assets/Audio/");
                            assetTypeToImport = "audio";
                        } else if (ext == ".ttf" || ext == ".otf") {
                            destDir = std::filesystem::absolute("../assets/Fonts/");
                            assetTypeToImport = "font";
                        } else if (ext == ".lua") {
                            destDir = std::filesystem::absolute("../assets/Scripts/");
                            assetTypeToImport = "script";
                        } else if (ext == ".json") { // Assuming .json for animations for now
                            // Check if it's in an 'animations' subdirectory or has a typical animation name
                            if (p.parent_path().filename() == "animations" || filename.find("anim") != std::string::npos) {
                                destDir = std::filesystem::absolute("../assets/Animations/");
                                assetTypeToImport = "animation";
                            } else {
                                // Generic json, could be scene or other data
                                destDir = std::filesystem::absolute("../assets/Scenes/"); // Or a generic data folder
                                assetTypeToImport = "scene_data";
                            }
                        } else {
                            addLogToConsole("Unsupported file type for import: " + ext);
                        }

                        if (!destDir.empty() && !assetTypeToImport.empty() && assetTypeToImport != "scene_data") {
                            std::filesystem::create_directories(destDir);
                            std::filesystem::path destPath = destDir / filename;
                            try {
                                std::filesystem::copy_file(p, destPath, std::filesystem::copy_options::overwrite_existing);
                                addLogToConsole("Imported " + assetTypeToImport + ": " + filename + " to " + destPath.string());
                                // Reload the specific asset type
                                if (assetTypeToImport == "texture") {
                                    assetManager.loadTexture(assetId, destPath.string());
                                } else if (assetTypeToImport == "audio") {
                                    assetManager.loadSound(assetId, destPath.string());
                                } else if (assetTypeToImport == "font") {
                                    assetManager.loadFont(assetId + "_16", destPath.string(), 16); // Default size
                                } else if (assetTypeToImport == "animation") {
                                    // assetManager.loadAnimation(assetId, destPath.string());
                                    addLogToConsole("Animation import for '" + assetId + "' needs AssetManager::loadAnimation implementation.");
                                }
                            } catch (const std::filesystem::filesystem_error& e) {
                                addLogToConsole("Error importing file: " + std::string(e.what()));
                            }
                        } else if (assetTypeToImport == "scene_data") {
                             addLogToConsole("Scene data files (.json) should be handled by scene loading logic, not direct asset import.");
                        }
                    }
                }
            }
            ImGui::Separator();

            // Asset Browser Tree
            std::function<void(const std::filesystem::path&, const std::string&)> displayDirectoryAssets = 
                [&](const std::filesystem::path& dirPath, const std::string& assetType) {
                if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
                    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                        const auto& path = entry.path();
                        std::string filename = path.filename().string();
                        std::string fullPath = std::filesystem::absolute(path).string();

                        if (entry.is_directory()) {
                            if (ImGui::TreeNode(filename.c_str())) {
                                displayDirectoryAssets(path, assetType); // Recurse
                                ImGui::TreePop();
                            }
                        } else {
                            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                            if (selectedAssetPathForPreview == fullPath) {
                                nodeFlags |= ImGuiTreeNodeFlags_Selected;
                            }
                            ImGui::TreeNodeEx(filename.c_str(), nodeFlags);
                            if (ImGui::IsItemClicked()) {
                                selectedAssetPathForPreview = fullPath;
                                selectedAssetTypeForPreview = assetType;
                                std::cout << "Selected asset: " << selectedAssetPathForPreview << " type: " << selectedAssetTypeForPreview << std::endl; // Debug log
                            }

                            if (assetType == "texture" && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                                ImGui::SetDragDropPayload("ASSET_TEXTURE_ID", filename.c_str(), filename.length() + 1);
                                ImGui::Text("%s", filename.c_str()); // Display item name while dragging
                                ImGui::EndDragDropSource();
                            }
                        }
                    }
                }
            };

            if (ImGui::CollapsingHeader("Textures")) {
                displayDirectoryAssets("../assets/Textures/", "texture");
            }
            if (ImGui::CollapsingHeader("Audio")) {
                displayDirectoryAssets("../assets/Audio/", "audio");
            }
            if (ImGui::CollapsingHeader("Animations")) {
                displayDirectoryAssets("../assets/Animations/", "animation");
            }
            if (ImGui::CollapsingHeader("Fonts")) {
                displayDirectoryAssets("../assets/Fonts/", "font");
            }
            if (ImGui::CollapsingHeader("Scripts")) {
                displayDirectoryAssets("../assets/Scripts/", "script");
            }
            if (ImGui::CollapsingHeader("Scenes")) {
                displayDirectoryAssets("../assets/Scenes/", "scene");
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

Entity DevModeScene::findEntityByName(const std::string& name) {
    for (auto const& entity : entityManager->getActiveEntities()) { 
        if (componentManager->hasComponent<NameComponent>(entity)) {
            auto& nameComp = componentManager->getComponent<NameComponent>(entity);
            if (nameComp.name == name) {
                return entity;
            }
        }
    }
    return NO_ENTITY_SELECTED;
}
