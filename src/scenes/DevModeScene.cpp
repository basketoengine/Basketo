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
      gameWindow(nullptr),
      gameRenderer(nullptr),
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
    componentManager->registerComponent<SoundEffectsComponent>();
    componentManager->registerComponent<RigidbodyComponent>();
    componentManager->registerComponent<CameraComponent>();
    componentManager->registerComponent<ParticleEmitterComponent>();
    componentManager->registerComponent<ParticleComponent>();
    componentManager->registerComponent<EventComponent>();
    componentManager->registerComponent<StateMachineComponent>();
    componentManager->registerComponent<UIComponent>();
    componentManager->registerComponent<UIButtonComponent>();
    componentManager->registerComponent<UITextComponent>();
    componentManager->registerComponent<UISliderComponent>();
    componentManager->registerComponent<UIInputFieldComponent>();
    componentManager->registerComponent<UIPanelComponent>();
    componentManager->registerComponent<UIImageComponent>();
    loadDevModeScene(*this, sceneFilePath);

    renderSystem = systemManager->registerSystem<RenderSystem>();
    movementSystem = systemManager->registerSystem<MovementSystem>();
    scriptSystem = systemManager->registerSystem<ScriptSystem>(entityManager.get(), componentManager.get());
    animationSystem = systemManager->registerSystem<AnimationSystem>();
    audioSystem = systemManager->registerSystem<AudioSystem>();
    cameraSystem = systemManager->registerSystem<CameraSystem>(componentManager.get(), entityManager.get(), renderer);
    collisionSystem = systemManager->registerSystem<CollisionSystem>();
    physicsSystem = systemManager->registerSystem<PhysicsSystem>();
    particleSystem = systemManager->registerSystem<ParticleSystem>();
    eventSystem = systemManager->registerSystem<EventSystem>();
    stateMachineSystem = systemManager->registerSystem<StateMachineSystem>();
    uiSystem = systemManager->registerSystem<UISystem>();

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

    Signature particleSig;
    particleSig.set(componentManager->getComponentType<TransformComponent>());
    particleSig.set(componentManager->getComponentType<ParticleEmitterComponent>());
    particleSig.set(componentManager->getComponentType<ParticleComponent>());
    systemManager->setSignature<ParticleSystem>(particleSig);

    Signature eventSig;
    eventSig.set(componentManager->getComponentType<EventComponent>());
    systemManager->setSignature<EventSystem>(eventSig);

    Signature stateMachineSig;
    stateMachineSig.set(componentManager->getComponentType<StateMachineComponent>());
    systemManager->setSignature<StateMachineSystem>(stateMachineSig);

    Signature uiSig;
    uiSig.set(componentManager->getComponentType<UIComponent>());
    systemManager->setSignature<UISystem>(uiSig);

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

    // Initialize separate game window if enabled
    if (useSeperateGameWindow) {
        if (!initGameWindow()) {
            std::cerr << "Failed to initialize separate game window, falling back to integrated view" << std::endl;
            useSeperateGameWindow = false;
        }
    }

}

DevModeScene::~DevModeScene() {
    Mix_HaltMusic();
    Mix_HaltChannel(-1);

    // Cleanup separate game window
    if (useSeperateGameWindow) {
        cleanupGameWindow();
    }

    std::cout << "Exiting Dev Mode Scene" << std::endl;
}

void DevModeScene::handleInput(SDL_Event& event) {
    // Handle game window events - game window is read-only, only handle window management events
    if (useSeperateGameWindow && gameWindow && event.type == SDL_WINDOWEVENT) {
        Uint32 gameWindowID = SDL_GetWindowID(gameWindow);
        if (event.window.windowID == gameWindowID) {
            switch (event.window.event) {
                case SDL_WINDOWEVENT_CLOSE:
                    // User closed game window, disable separate window mode
                    useSeperateGameWindow = false;
                    cleanupGameWindow();
                    addLogToConsole("Game window closed, switching to integrated view");
                    break;
            }
            return; // Don't process this event further - game window is read-only
        }
    }

    // Filter out any mouse/keyboard events that target the game window
    if (useSeperateGameWindow && gameWindow &&
        (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
         event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEWHEEL ||
         event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)) {

        Uint32 gameWindowID = SDL_GetWindowID(gameWindow);
        Uint32 eventWindowID = 0;

        // Get the window ID for mouse events
        if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            eventWindowID = event.button.windowID;
        } else if (event.type == SDL_MOUSEMOTION) {
            eventWindowID = event.motion.windowID;
        } else if (event.type == SDL_MOUSEWHEEL) {
            eventWindowID = event.wheel.windowID;
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            eventWindowID = event.key.windowID;
        }

        // If the event is targeting the game window, ignore it (game window is read-only)
        if (eventWindowID == gameWindowID) {
            return;
        }
    }

    // Only process input for the main editor window
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

    // 7. Events
    if (eventSystem) {
        eventSystem->update(componentManager.get(), deltaTime);
    }

    // 8. State Machines
    if (stateMachineSystem) {
        // Connect event system to state machine system
        if (eventSystem) {
            stateMachineSystem->setEventSystem(eventSystem.get());
        }
        stateMachineSystem->update(componentManager.get(), deltaTime);
    }

    // 9. UI System
    if (uiSystem) {
        uiSystem->update(componentManager.get(), deltaTime);
    }

    // 10. Particles
    if (particleSystem) {
        particleSystem->update(componentManager.get(), deltaTime);
    }

    // Camera system is only updated in game view, not in scene editor
    // Scene editor uses its own independent camera (cameraX, cameraY, cameraZoom)
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

    // Scene editor always uses its own independent camera (never affected by game camera)
    // This allows editing even during play mode - scene view and game view are completely separate
    float currentRenderCameraX = this->cameraX;
    float currentRenderCameraY = this->cameraY;
    float currentRenderZoom = this->cameraZoom;

    // Ensure panel sizes are within reasonable bounds
    float maxHierarchyWidth = displaySize.x - minGameViewWidth - minInspectorWidth;
    float maxInspectorWidth = displaySize.x - minGameViewWidth - minHierarchyWidth;
    float maxBottomPanelHeight = displaySize.y - topToolbarHeight - minGameViewHeight;

    hierarchyWidth = std::max(minHierarchyWidth, std::min(maxHierarchyWidth, hierarchyWidth));
    inspectorWidth = std::max(minInspectorWidth, std::min(maxInspectorWidth, inspectorWidth));
    bottomPanelHeight = std::max(minBottomPanelHeight, std::min(maxBottomPanelHeight, bottomPanelHeight));

    gameViewport = {
        static_cast<int>(hierarchyWidth),
        static_cast<int>(topToolbarHeight),
        static_cast<int>(displaySize.x - hierarchyWidth - inspectorWidth),
        static_cast<int>(displaySize.y - topToolbarHeight - bottomPanelHeight)
    };
    if (gameViewport.w < 0) gameViewport.w = 0;
    if (gameViewport.h < 0) gameViewport.h = 0;

    // Create dockspace for proper panel management
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Initialize docking layout asynchronously after a few frames
    static int frameCount = 0;
    frameCount++;

    if (!dockingLayoutInitialized && frameCount > 3) {
        dockingLayoutInitialized = true;

        // Only initialize if the dockspace exists and is valid
        if (ImGui::DockBuilderGetNode(dockspace_id) != nullptr) {
            // Clear any existing layout
            ImGui::DockBuilderRemoveNode(dockspace_id);
        }

        // Add the main dockspace node
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, displaySize);

        // Split the dockspace into regions with proper ratios
        ImGuiID dock_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
        ImGuiID dock_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.35f, nullptr, &dockspace_id);
        ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);

        // Dock windows to their respective areas
        ImGui::DockBuilderDockWindow("Hierarchy", dock_left);
        ImGui::DockBuilderDockWindow("Inspector", dock_right);
        ImGui::DockBuilderDockWindow("Assets", dock_bottom);
        ImGui::DockBuilderDockWindow("GameViewport", dockspace_id); // Center

        // Finish the docking setup
        ImGui::DockBuilderFinish(dockspace_id);
    }

    // Always create the scene viewport in the main window (this is the Scene Editor view)
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(gameViewport.x), static_cast<float>(gameViewport.y)), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(gameViewport.w), static_cast<float>(gameViewport.h)), ImGuiCond_Always);
    ImGuiWindowFlags viewportFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
    if (!ImGui::IsDragDropActive()) {
        viewportFlags |= ImGuiWindowFlags_NoInputs;
    }

    // Change window title based on mode
    const char* windowTitle = useSeperateGameWindow ? "Scene Editor" : "GameViewport";
    ImGui::Begin(windowTitle, nullptr, viewportFlags);

    // Allow drag-and-drop in scene editor - this is where you edit the scene
    // The game view window (separate window) is the one that should be read-only
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

                // Reload game textures to ensure new sprites appear in game view
                reloadGameTextures();
            }
            ImGui::EndDragDropTarget();
        }
    }

    // Add visual indicator when using separate game window
    if (useSeperateGameWindow) {
        ImGui::SetCursorPos(ImVec2(10, 30));
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "SCENE EDITOR");
        ImGui::SetCursorPos(ImVec2(10, 45));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Game view in separate window");
    }

    ImGui::End();
    SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer);

    // Always render scene content in main window (this is the Scene Editor view)
    // Scene editor always shows grid and editing tools, regardless of play state
    if (gameViewport.w > 0 && gameViewport.h > 0) {
        SDL_RenderSetViewport(renderer, &gameViewport);

        // Always show grid in scene editor (not affected by play mode)
        if (showGrid) {
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

        // Always show colliders in scene editor (for editing purposes, regardless of play mode)
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


        // Always show selection in scene editor (for editing purposes, regardless of play mode)
        if (selectedEntity != NO_ENTITY_SELECTED && componentManager->hasComponent<TransformComponent>(selectedEntity)) {
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

    // "⋮" button for file operations popup menu (vertical ellipsis)
    if (ImGui::Button("\u22EE")) { // Unicode U+22EE Vertical Ellipsis
        ImGui::OpenPopup("file_operations_popup");
    }
    // Popup menu for file operations (except Load)
    if (ImGui::BeginPopup("file_operations_popup")) {
        if (ImGui::MenuItem("Save")) { saveDevModeScene(*this, sceneFilePath); }
        if (ImGui::MenuItem("Save As...")) { /* TODO: Implement Save As */ }
        if (ImGui::MenuItem("New Scene")) { createNewScene(); }
        ImGui::Separator();
        if (ImGui::MenuItem("Import...")) { /* TODO: Implement Import */ }
        if (ImGui::MenuItem("Export...")) { /* TODO: Implement Export */ }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    // Filename input
    ImGui::PushItemWidth(120);
    ImGui::InputText("##Filename", sceneFilePath, sizeof(sceneFilePath));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    // Load button directly in toolbar
    if (ImGui::Button("Load")) { loadDevModeScene(*this, sceneFilePath); }
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

    // Play/Stop button
    if (isPlaying) {
        if (ImGui::Button("Stop")) {
            isPlaying = false;
            Mix_HaltMusic();
            Mix_HaltChannel(-1);
            loadDevModeScene(*this, sceneFilePath);
        }
    } else {
        if (ImGui::Button("Play")) {
            // Restore all components from the scene file before entering play mode
            loadDevModeScene(*this, sceneFilePath);
            isPlaying = true;
            selectedEntity = NO_ENTITY_SELECTED;
            // Reload scripts to ensure they are initialized for the play session
            for (auto entity : entityManager->getActiveEntities()) {
                if (componentManager->hasComponent<ScriptComponent>(entity)) {
                    auto& scriptComp = componentManager->getComponent<ScriptComponent>(entity);
                    if (!scriptComp.scriptPath.empty() && scriptSystem) {
                        scriptSystem->loadScript(entity, scriptComp.scriptPath);
                    }
                }
            }
            // Reset animation states
            if (animationSystem && entityManager && componentManager) {
                for (auto entity : entityManager->getActiveEntities()) {
                    if (componentManager->hasComponent<AnimationComponent>(entity)) {
                        auto& animComp = componentManager->getComponent<AnimationComponent>(entity);
                        animComp.currentFrameTime = 0.0f;
                        animComp.currentFrameIndex = 0;
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

    // Toggle for separate game window
    if (ImGui::Checkbox("Separate Game Window", &useSeperateGameWindow)) {
        if (useSeperateGameWindow) {
            if (!initGameWindow()) {
                addLogToConsole("Failed to create separate game window");
                useSeperateGameWindow = false;
            }
        } else {
            cleanupGameWindow();
            addLogToConsole("Separate game window disabled");
        }
    }
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

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
                // Reload game textures to ensure new sprites appear in game view
                reloadGameTextures();
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

                    // Special label for game camera
                    if (entity == gameCameraEntity) {
                        label = "🎥 Game Camera (" + std::to_string(entity) + ")";
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

    // Render hierarchy splitter - ensure it's in the main window context
    ImGui::SetCurrentContext(ImGui::GetCurrentContext());
    float maxHierarchyWidthForSplitter = displaySize.x - minGameViewWidth - minInspectorWidth;
    renderVerticalSplitter("HierarchySplitter", hierarchyWidth, minHierarchyWidth, maxHierarchyWidthForSplitter,
                          hierarchyWidth, topToolbarHeight, displaySize.y - topToolbarHeight - bottomPanelHeight);

    EditorUI::renderInspectorPanel(*this, io);

    // Render inspector splitter - ensure it's in the main window context
    float maxInspectorWidthForSplitter = displaySize.x - minGameViewWidth - minHierarchyWidth;
    renderVerticalSplitter("InspectorSplitter", inspectorWidth, minInspectorWidth, maxInspectorWidthForSplitter,
                          hierarchyWidth + gameViewport.w, topToolbarHeight, displaySize.y - topToolbarHeight - bottomPanelHeight);

    // Asset Preview Panel (Bottom Right)
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
                        } else if (ext == ".json") {
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
                                // Use texture ID without extension for consistency with AssetManager
                                std::string textureId = std::filesystem::path(filename).stem().string();
                                ImGui::SetDragDropPayload("ASSET_TEXTURE_ID", textureId.c_str(), textureId.length() + 1);
                                ImGui::Text("%s", filename.c_str()); // Display full filename while dragging
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

    // Render bottom panel splitter
    float maxBottomPanelHeightForSplitter = displaySize.y - topToolbarHeight - minGameViewHeight;
    renderHorizontalSplitter("BottomSplitter", bottomPanelHeight, minBottomPanelHeight, maxBottomPanelHeightForSplitter,
                            0, displaySize.y - bottomPanelHeight, displaySize.x);

    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, SDL_GL_GetCurrentContext());
    }

    SDL_RenderPresent(renderer);

    // Render to separate game window if enabled
    if (useSeperateGameWindow) {
        renderGameWindow();
    }
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

void DevModeScene::createNewScene() {
    const char* title = "New Scene";
    const char* message = "Enter new scene name (e.g., my_scene). It will be saved in ../assets/Scenes/ with .json extension.";
    const char* defaultInput = "new_scene";
    const char* inputSceneName = tinyfd_inputBox(title, message, defaultInput);

    std::string sceneNameStr;
    if (inputSceneName == NULL || strlen(inputSceneName) == 0) {
        addLogToConsole("New scene name cancelled or empty, using default 'new_scene.json'.");
        sceneNameStr = "new_scene";
    } else {
        sceneNameStr = inputSceneName;
    }

    std::string baseName = sceneNameStr;
    if (baseName.length() > 5 && baseName.substr(baseName.length() - 5) == ".json") {
        baseName = baseName.substr(0, baseName.length() - 5);
    }
    std::string newScenePath = "../assets/Scenes/" + baseName + ".json";

    if (entityManager) entityManager->clear();
  
    selectedEntity = NO_ENTITY_SELECTED;
    inspectorTextureIdBuffer[0] = '\0';
    inspectorScriptPathBuffer[0] = '\0';
    cameraX = 0.0f;
    cameraY = 0.0f;
    cameraZoom = 1.0f;
    consoleLogBuffer.clear(); 
    addLogToConsole("Cleared current scene data for new scene.");

    strncpy(sceneFilePath, newScenePath.c_str(), sizeof(sceneFilePath) - 1);
    sceneFilePath[sizeof(sceneFilePath) - 1] = '\0';

    nlohmann::json emptySceneJson;
    emptySceneJson["entities"] = nlohmann::json::array();
    std::ofstream outFile(sceneFilePath);
    if (outFile.is_open()) {
        outFile << emptySceneJson.dump(4);
        outFile.close();
        addLogToConsole("Created new empty scene: " + std::string(sceneFilePath));
    } else {
        addLogToConsole("Error: Could not create new scene file: " + std::string(sceneFilePath));
        const char* fallbackScene = "../assets/Scenes/scene.json";
        strncpy(sceneFilePath, fallbackScene, sizeof(sceneFilePath) -1);
        sceneFilePath[sizeof(sceneFilePath) -1] = '\0';
        loadDevModeScene(*this, sceneFilePath); 
        return;
    }

    if (!loadDevModeScene(*this, sceneFilePath)) {
        addLogToConsole("Error: Failed to load the new scene: " + std::string(sceneFilePath) + ". Check console for details.");
        const char* fallbackScene = "../assets/Scenes/scene.json";
        strncpy(sceneFilePath, fallbackScene, sizeof(sceneFilePath) -1);
        sceneFilePath[sizeof(sceneFilePath) -1] = '\0';
        loadDevModeScene(*this, sceneFilePath);
    }
}

bool DevModeScene::initGameWindow() {
    int editorX, editorY, editorW, editorH;
    SDL_GetWindowPosition(window, &editorX, &editorY);
    SDL_GetWindowSize(window, &editorW, &editorH);

    int gameWindowX = editorX + editorW + 10; 
    int gameWindowY = editorY;
    int gameWindowW = 800;
    int gameWindowH = 600;

    gameWindow = SDL_CreateWindow("Game View (Read-Only)",
                                  gameWindowX, gameWindowY,
                                  gameWindowW, gameWindowH,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!gameWindow) {
        std::cerr << "Failed to create game window: " << SDL_GetError() << std::endl;
        return false;
    }

    gameRenderer = SDL_CreateRenderer(gameWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!gameRenderer) {
        std::cerr << "Failed to create game renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(gameWindow);
        gameWindow = nullptr;
        return false;
    }

    // Load textures for the game renderer
    loadTexturesForGameRenderer();

    // Create default camera for game view
    createDefaultGameCamera();

    addLogToConsole("Separate game window initialized successfully");
    return true;
}

void DevModeScene::cleanupGameWindow() {
    // Clean up game textures (avoid destroying same texture multiple times)
    std::set<SDL_Texture*> uniqueTextures;
    for (auto& pair : gameTextures) {
        if (pair.second) {
            uniqueTextures.insert(pair.second);
        }
    }
    for (SDL_Texture* texture : uniqueTextures) {
        SDL_DestroyTexture(texture);
    }
    gameTextures.clear();

    if (gameRenderer) {
        SDL_DestroyRenderer(gameRenderer);
        gameRenderer = nullptr;
    }

    if (gameWindow) {
        SDL_DestroyWindow(gameWindow);
        gameWindow = nullptr;
    }
}

void DevModeScene::renderGameWindow() {
    if (!gameWindow || !gameRenderer) return;

    int gameWindowW, gameWindowH;
    SDL_GetWindowSize(gameWindow, &gameWindowW, &gameWindowH);

    SDL_Rect gameViewportRect = {0, 0, gameWindowW, gameWindowH};
    SDL_RenderSetViewport(gameRenderer, &gameViewportRect);

    float currentRenderCameraX = 0.0f;
    float currentRenderCameraY = 0.0f;
    float currentRenderZoom = 1.0f;

    if (gameCameraEntity != 0 &&
        componentManager->hasComponent<TransformComponent>(gameCameraEntity) &&
        componentManager->hasComponent<CameraComponent>(gameCameraEntity)) {

        auto& cameraTransform = componentManager->getComponent<TransformComponent>(gameCameraEntity);
        auto& cameraComp = componentManager->getComponent<CameraComponent>(gameCameraEntity);

        if (cameraComp.isActive) {
            currentRenderCameraX = cameraTransform.x;
            currentRenderCameraY = cameraTransform.y;
            currentRenderZoom = cameraComp.zoom;
        }
    }

    if (isPlaying && cameraSystem) {
        SDL_Rect activeGameCameraWorldView;
        float gameCamZoom = 1.0f;
        cameraSystem->update(activeGameCameraWorldView, gameCamZoom);
        Entity activeCamEntity = cameraSystem->getActiveCameraEntity();

        if (activeCamEntity != NO_ENTITY) {
            currentRenderCameraX = static_cast<float>(activeGameCameraWorldView.x);
            currentRenderCameraY = static_cast<float>(activeGameCameraWorldView.y);
            currentRenderZoom = gameCamZoom;
        }
    }

    SDL_SetRenderDrawColor(gameRenderer,
                          (Uint8)(clear_color.x * 255),
                          (Uint8)(clear_color.y * 255),
                          (Uint8)(clear_color.z * 255),
                          (Uint8)(clear_color.w * 255));
    SDL_RenderClear(gameRenderer);

    std::vector<Entity> entitiesToRender;
    if (entityManager) {
        for (auto entity : entityManager->getActiveEntities()) {
            if (componentManager && componentManager->hasComponent<TransformComponent>(entity) && componentManager->hasComponent<SpriteComponent>(entity)) {
                entitiesToRender.push_back(entity);
            }
        }
    }

    static int debugCounter = 0;
    if (debugCounter % 60 == 0) {
        std::cout << "Game window rendering " << entitiesToRender.size() << " entities" << std::endl;
    }
    debugCounter++;

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

        SDL_Texture* texture = nullptr;
        auto gameTextureIt = gameTextures.find(sprite.textureId);
        if (gameTextureIt != gameTextures.end()) {
            texture = gameTextureIt->second;
        } else {
            texture = AssetManager::getInstance().getTexture(sprite.textureId);
        }

        if (!texture) {
            static std::set<std::string> loggedMissingTextures;
            if (loggedMissingTextures.find(sprite.textureId) == loggedMissingTextures.end()) {
                std::cout << "Game window: Missing texture for entity " << entity << " with textureId: '" << sprite.textureId << "'" << std::endl;
                loggedMissingTextures.insert(sprite.textureId);
            }
            continue;
        }

        SDL_Rect destRect;
        destRect.x = (int)((transform.x - currentRenderCameraX) * currentRenderZoom);
        destRect.y = (int)((transform.y - currentRenderCameraY) * currentRenderZoom);
        destRect.w = (int)(transform.width * currentRenderZoom);
        destRect.h = (int)(transform.height * currentRenderZoom);

        if (entity == entitiesToRender[0] && debugCounter % 120 == 0) {
            std::cout << "Game window: Rendering entity " << entity << " at (" << destRect.x << "," << destRect.y << ") size (" << destRect.w << "," << destRect.h << ")" << std::endl;
            std::cout << "  Transform: (" << transform.x << "," << transform.y << ") Camera: (" << currentRenderCameraX << "," << currentRenderCameraY << ") Zoom: " << currentRenderZoom << std::endl;
        }

        SDL_Rect* srcRectPtr = sprite.useSrcRect ? &sprite.srcRect : nullptr;
        SDL_Point center = { (int)(transform.width * currentRenderZoom / 2), (int)(transform.height * currentRenderZoom / 2) };
        SDL_RenderCopyEx(gameRenderer, texture, srcRectPtr, &destRect, transform.rotation, &center, sprite.flip);
    }

    // Render particles
    if (particleSystem) {
        particleSystem->render(gameRenderer, componentManager.get(), currentRenderCameraX, currentRenderCameraY);
    }

    // Render UI (always on top)
    if (uiSystem) {
        uiSystem->render(gameRenderer, componentManager.get());
    }

    SDL_RenderSetViewport(gameRenderer, nullptr);
    SDL_RenderPresent(gameRenderer);
}

void DevModeScene::loadTexturesForGameRenderer() {
    if (!gameRenderer) return;

    addLogToConsole("Loading textures for game window renderer...");

    // Clear existing game textures (avoid destroying same texture multiple times)
    std::set<SDL_Texture*> uniqueTextures;
    for (auto& pair : gameTextures) {
        if (pair.second) {
            uniqueTextures.insert(pair.second);
        }
    }
    for (SDL_Texture* texture : uniqueTextures) {
        SDL_DestroyTexture(texture);
    }
    gameTextures.clear();

    std::string texturesRoot = "../assets/Textures/";
    namespace fs = std::filesystem;
    if (fs::exists(texturesRoot)) {
        for (const auto& entry : fs::recursive_directory_iterator(texturesRoot)) {
            if (entry.is_regular_file()) {
                std::string fullPath = entry.path().string();
                std::string relPath = fs::relative(entry.path(), texturesRoot).string();

                // Load the texture for the game renderer
                SDL_Surface* surface = IMG_Load(fullPath.c_str());
                if (surface) {
                    SDL_Texture* gameTexture = SDL_CreateTextureFromSurface(gameRenderer, surface);
                    if (gameTexture) {
                        // Store texture with both full filename (with extension) and stem (without extension)
                        // This handles both texture ID formats used by different loading paths
                        std::string filenameWithExt = relPath;
                        std::string filenameWithoutExt = std::filesystem::path(relPath).stem().string();

                        gameTextures[filenameWithExt] = gameTexture;
                        gameTextures[filenameWithoutExt] = gameTexture; // Same texture, different key

                        std::cout << "Loaded texture '" << filenameWithExt << "' (also as '" << filenameWithoutExt << "') for game renderer" << std::endl;
                    } else {
                        std::cerr << "Failed to create game texture from surface: " << SDL_GetError() << std::endl;
                    }
                    SDL_FreeSurface(surface);
                } else {
                    std::cerr << "Failed to load surface for game renderer: " << fullPath << " - " << IMG_GetError() << std::endl;
                }
            }
        }
    }

    addLogToConsole("Finished loading textures for game window");
}

void DevModeScene::reloadGameTextures() {
    if (useSeperateGameWindow && gameRenderer) {
        loadTexturesForGameRenderer();
    }
}

void DevModeScene::createDefaultGameCamera() {
    if (gameCameraEntity != 0 && componentManager->hasComponent<TransformComponent>(gameCameraEntity)) {
        return;
    }

    gameCameraEntity = entityManager->createEntity();

    TransformComponent cameraTransform;
    cameraTransform.x = 0.0f;
    cameraTransform.y = 0.0f;
    cameraTransform.z_index = 0;
    componentManager->addComponent(gameCameraEntity, cameraTransform);

    CameraComponent cameraComp;
    cameraComp.zoom = 1.0f;
    cameraComp.isActive = true;
    componentManager->addComponent(gameCameraEntity, cameraComp);

    addLogToConsole("Created default game camera entity: " + std::to_string(gameCameraEntity));
}

// Panel resizing helpers
bool DevModeScene::renderVerticalSplitter(const char* id, float& size, float minSize, float maxSize, float x, float y, float height) {
    // Use foreground draw list to ensure splitter is always visible
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // Create invisible button for interaction
    ImGui::SetCursorScreenPos(ImVec2(x - 2, y));
    ImGui::InvisibleButton(id, ImVec2(4, height));

    bool isHovered = ImGui::IsItemHovered();
    bool isActive = ImGui::IsItemActive();

    if (isHovered || isActive) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float delta = ImGui::GetIO().MouseDelta.x;
        size += delta;
        size = std::max(minSize, std::min(maxSize, size));
        return true;
    }

    // Draw splitter line with more visible colors
    ImU32 color = isActive ? IM_COL32(150, 150, 255, 255) :
                  isHovered ? IM_COL32(120, 120, 120, 255) :
                  IM_COL32(80, 80, 80, 255);
    drawList->AddLine(ImVec2(x, y), ImVec2(x, y + height), color, 2.0f);

    return false;
}

bool DevModeScene::renderHorizontalSplitter(const char* id, float& size, float minSize, float maxSize, float x, float y, float width) {
    // Use foreground draw list to ensure splitter is always visible
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // Create invisible button for interaction
    ImGui::SetCursorScreenPos(ImVec2(x, y - 2));
    ImGui::InvisibleButton(id, ImVec2(width, 4));

    bool isHovered = ImGui::IsItemHovered();
    bool isActive = ImGui::IsItemActive();

    if (isHovered || isActive) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }

    if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        float delta = ImGui::GetIO().MouseDelta.y;
        size -= delta; // Subtract because we're measuring from bottom
        size = std::max(minSize, std::min(maxSize, size));
        return true;
    }

    // Draw splitter line with more visible colors
    ImU32 color = isActive ? IM_COL32(150, 150, 255, 255) :
                  isHovered ? IM_COL32(120, 120, 120, 255) :
                  IM_COL32(80, 80, 80, 255);
    drawList->AddLine(ImVec2(x, y), ImVec2(x + width, y), color, 2.0f);

    return false;
}
