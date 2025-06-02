\
#include "InspectorPanel.h"
#include "DevModeScene.h" 
#include "imgui.h"
#include "../../vendor/imgui/backends/imgui_impl_sdl2.h"
#include "../../vendor/imgui/backends/imgui_impl_sdlrenderer2.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/VelocityComponent.h"
#include "../ecs/components/ScriptComponent.h"
#include "../ecs/components/ColliderComponent.h"
#include "../ecs/components/AnimationComponent.h" 
#include "../ecs/components/AudioComponent.h" 
#include "../ecs/components/CameraComponent.h"
#include "../ecs/components/RigidbodyComponent.h"
#include "../AssetManager.h"
#include "../utils/FileUtils.h" 
#include "../utils/EditorHelpers.h" 
#include "tinyfiledialogs.h"
#include <filesystem> 
#include <iostream> 
#include <string> 
#include <vector> 
#include <algorithm> 

namespace EditorUI {

void renderInspectorPanel(DevModeScene& scene, ImGuiIO& io) {
    ImVec2 displaySize = io.DisplaySize;
    const float inspectorWidth = displaySize.x * scene.inspectorWidthRatio;
    const float bottomPanelHeight = displaySize.y * scene.bottomPanelHeightRatio;
    const ImGuiWindowFlags fixedPanelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    ImGui::SetNextWindowPos(ImVec2(displaySize.x - inspectorWidth, scene.topToolbarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(inspectorWidth, displaySize.y - scene.topToolbarHeight - bottomPanelHeight), ImGuiCond_Always);
    ImGui::Begin("Inspector", nullptr, fixedPanelFlags);

    if (scene.selectedEntity != NO_ENTITY_SELECTED && std::find(scene.entityManager->getActiveEntities().begin(), scene.entityManager->getActiveEntities().end(), scene.selectedEntity) != scene.entityManager->getActiveEntities().end()) {
        ImGui::Text("Selected Entity: %u", scene.selectedEntity);
        ImGui::Separator();

        ImGui::PushItemWidth(-1);
        const char* component_types[] = { "Transform", "Sprite", "Velocity", "Script", "Collider", "Animation", "Audio", "SoundEffects", "Camera", "Rigidbody" };
        static int current_component_type_idx = 0;

        if (ImGui::BeginCombo("##AddComponentCombo", component_types[current_component_type_idx])) {
            for (int n = 0; n < IM_ARRAYSIZE(component_types); n++) {
                const bool is_selected = (current_component_type_idx == n);
                if (ImGui::Selectable(component_types[n], is_selected))
                    current_component_type_idx = n;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if (ImGui::Button("Add Selected Component", ImVec2(-1, 0))) {
            std::string selected_component_str = component_types[current_component_type_idx];
            Signature entitySignature = scene.entityManager->getSignature(scene.selectedEntity);

            if (selected_component_str == "Transform") {
                if (!scene.componentManager->hasComponent<TransformComponent>(scene.selectedEntity)) {
                    scene.componentManager->addComponent(scene.selectedEntity, TransformComponent{});
                    entitySignature.set(scene.componentManager->getComponentType<TransformComponent>());
                    std::cout << "Added TransformComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has TransformComponent." << std::endl;
                }
            } else if (selected_component_str == "Sprite") {
                if (!scene.componentManager->hasComponent<SpriteComponent>(scene.selectedEntity)) {
                    scene.componentManager->addComponent(scene.selectedEntity, SpriteComponent{});
                    entitySignature.set(scene.componentManager->getComponentType<SpriteComponent>());
                    std::cout << "Added SpriteComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has SpriteComponent." << std::endl;
                }
            } else if (selected_component_str == "Velocity") {
                if (!scene.componentManager->hasComponent<VelocityComponent>(scene.selectedEntity)) {
                    scene.componentManager->addComponent(scene.selectedEntity, VelocityComponent{});
                    entitySignature.set(scene.componentManager->getComponentType<VelocityComponent>());
                    std::cout << "Added VelocityComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has VelocityComponent." << std::endl;
                }
            } else if (selected_component_str == "Script") {
                if (addComponentIfMissing<ScriptComponent>(scene.componentManager.get(), scene.selectedEntity)) {
                    entitySignature.set(scene.componentManager->getComponentType<ScriptComponent>());
                    scene.inspectorScriptPathBuffer[0] = '\0';
                    std::cout << "Added ScriptComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has ScriptComponent." << std::endl;
                }
            } else if (selected_component_str == "Collider") {
                if (addComponentIfMissing<ColliderComponent>(scene.componentManager.get(), scene.selectedEntity)) {
                    entitySignature.set(scene.componentManager->getComponentType<ColliderComponent>());
                    std::cout << "Added ColliderComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has ColliderComponent." << std::endl;
                }
            } else if (selected_component_str == "Animation") { 
                if (addComponentIfMissing<AnimationComponent>(scene.componentManager.get(), scene.selectedEntity)) {
                    entitySignature.set(scene.componentManager->getComponentType<AnimationComponent>());
                    std::cout << "Added AnimationComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has AnimationComponent." << std::endl;
                }
            } else if (selected_component_str == "Audio") {
                if (!scene.componentManager->hasComponent<AudioComponent>(scene.selectedEntity)) {
                    scene.componentManager->addComponent(scene.selectedEntity, AudioComponent{});
                    entitySignature.set(scene.componentManager->getComponentType<AudioComponent>());
                    std::cout << "Added AudioComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has AudioComponent." << std::endl;
                }
            } else if (selected_component_str == "SoundEffects") {
                if (!scene.componentManager->hasComponent<SoundEffectsComponent>(scene.selectedEntity)) {
                    scene.componentManager->addComponent(scene.selectedEntity, SoundEffectsComponent{});
                    entitySignature.set(scene.componentManager->getComponentType<SoundEffectsComponent>());
                    std::cout << "Added SoundEffectsComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has SoundEffectsComponent." << std::endl;
                }
            } else if (selected_component_str == "Camera") {
                if (!scene.componentManager->hasComponent<CameraComponent>(scene.selectedEntity)) {
                    // When adding a new camera, if it's set to active, deactivate others.
                    CameraComponent newCamComp;
                    if (newCamComp.isActive) {
                        for (Entity entity : scene.entityManager->getActiveEntities()) {
                            if (scene.componentManager->hasComponent<CameraComponent>(entity) && entity != scene.selectedEntity) {
                                scene.componentManager->getComponent<CameraComponent>(entity).isActive = false;
                            }
                        }
                    }
                    scene.componentManager->addComponent(scene.selectedEntity, newCamComp);
                    entitySignature.set(scene.componentManager->getComponentType<CameraComponent>());
                    std::cout << "Added CameraComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has CameraComponent." << std::endl;
                }
            } else if (selected_component_str == "Rigidbody") {
                if (!scene.componentManager->hasComponent<RigidbodyComponent>(scene.selectedEntity)) {
                    scene.componentManager->addComponent(scene.selectedEntity, RigidbodyComponent{});
                    entitySignature.set(scene.componentManager->getComponentType<RigidbodyComponent>());
                    scene.entityManager->setSignature(scene.selectedEntity, entitySignature);
                    scene.systemManager->entitySignatureChanged(scene.selectedEntity, entitySignature);
                    std::cout << "Added RigidbodyComponent to Entity " << scene.selectedEntity << std::endl;
                } else {
                    std::cout << "Entity " << scene.selectedEntity << " already has RigidbodyComponent." << std::endl;
                }
            }

            scene.entityManager->setSignature(scene.selectedEntity, entitySignature);
            scene.systemManager->entitySignatureChanged(scene.selectedEntity, entitySignature);
        }
        ImGui::Separator();

        if (scene.selectedEntity != NO_ENTITY_SELECTED) {
            ImGui::Spacing();
            if (ImGui::Button("Delete Entity", ImVec2(-1, 0))) {
                Entity entityToDelete = scene.selectedEntity;
                scene.selectedEntity = NO_ENTITY_SELECTED;
                scene.inspectorTextureIdBuffer[0] = '\0';
                scene.inspectorScriptPathBuffer[0] = '\0';

                // Properly notify systems and components before destroying entity
                scene.systemManager->entityDestroyed(entityToDelete);
                scene.componentManager->entityDestroyed(entityToDelete);
                scene.entityManager->destroyEntity(entityToDelete);
                std::cout << "Deleted Entity " << entityToDelete << std::endl;
            }
            ImGui::Separator();
        }

        if (scene.componentManager->hasComponent<TransformComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Transform Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& transform = scene.componentManager->getComponent<TransformComponent>(scene.selectedEntity);
                ImGui::DragFloat("Position X##Transform", &transform.x, 1.0f);
                ImGui::DragFloat("Position Y##Transform", &transform.y, 1.0f);
                ImGui::DragFloat("Width##Transform", &transform.width, 1.0f, HANDLE_SIZE);
                ImGui::DragFloat("Height##Transform", &transform.height, 1.0f, HANDLE_SIZE);
                ImGui::DragFloat("Rotation##Transform", &transform.rotation, 1.0f, -360.0f, 360.0f);
                ImGui::DragInt("Z-Index##Transform", &transform.z_index);
            }
        } else ImGui::TextDisabled("No Transform Component");

        if (scene.componentManager->hasComponent<SpriteComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Sprite Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& sprite = scene.componentManager->getComponent<SpriteComponent>(scene.selectedEntity);
                if (scene.inspectorTextureIdBuffer[0] == '\0' || sprite.textureId != scene.inspectorTextureIdBuffer) {
                    strncpy(scene.inspectorTextureIdBuffer, sprite.textureId.c_str(), IM_ARRAYSIZE(scene.inspectorTextureIdBuffer) - 1);
                    scene.inspectorTextureIdBuffer[IM_ARRAYSIZE(scene.inspectorTextureIdBuffer) - 1] = '\0';
                }
                ImGui::Text("Texture ID/Path:");
                if (ImGui::InputText("##SpriteTexturePath", scene.inspectorTextureIdBuffer, IM_ARRAYSIZE(scene.inspectorTextureIdBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    std::string newTextureId(scene.inspectorTextureIdBuffer);
                    AssetManager& assets = AssetManager::getInstance();
                    if (assets.getTexture(newTextureId) || assets.loadTexture(newTextureId, newTextureId)) {
                        sprite.textureId = newTextureId;
                        // Reload game textures to ensure texture appears in game view
                        scene.reloadGameTextures();
                    } else {
                        std::cerr << "Inspector Error: Failed to find or load texture: '" << newTextureId << "'. Reverting." << std::endl;
                        strncpy(scene.inspectorTextureIdBuffer, sprite.textureId.c_str(), IM_ARRAYSIZE(scene.inspectorTextureIdBuffer) - 1);
                        scene.inspectorTextureIdBuffer[IM_ARRAYSIZE(scene.inspectorTextureIdBuffer) - 1] = '\0';
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
                            std::filesystem::path destDir = std::filesystem::absolute("../assets/Textures/");
                            std::filesystem::path destPath = destDir / filename;

                            try {
                                std::filesystem::create_directories(destDir);
                                std::filesystem::copy_file(selectedPath, destPath, std::filesystem::copy_options::overwrite_existing);
                                std::cout << "Asset copied to: " << destPath.string() << std::endl;

                                AssetManager& assets = AssetManager::getInstance();
                                if (assets.loadTexture(assetId, destPath.string())) {
                                    sprite.textureId = assetId;
                                    strncpy(scene.inspectorTextureIdBuffer, assetId.c_str(), IM_ARRAYSIZE(scene.inspectorTextureIdBuffer) - 1);
                                    scene.inspectorTextureIdBuffer[IM_ARRAYSIZE(scene.inspectorTextureIdBuffer) - 1] = '\0';
                                    std::cout << "Texture loaded and assigned: " << assetId << std::endl;

                                    // Reload game textures to ensure new texture appears in game view
                                    scene.reloadGameTextures();
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
        ImGui::Separator();

        if (scene.componentManager->hasComponent<VelocityComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Velocity Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& vel = scene.componentManager->getComponent<VelocityComponent>(scene.selectedEntity);
                ImGui::DragFloat("Velocity X", &vel.vx, 0.1f);
                ImGui::DragFloat("Velocity Y", &vel.vy, 0.1f);

                if (ImGui::Button("Remove Velocity Component")) {
                    scene.componentManager->removeComponent<VelocityComponent>(scene.selectedEntity);
                    Signature sig = scene.entityManager->getSignature(scene.selectedEntity);
                    sig.reset(scene.componentManager->getComponentType<VelocityComponent>());
                    scene.entityManager->setSignature(scene.selectedEntity, sig);
                    scene.systemManager->entitySignatureChanged(scene.selectedEntity, sig);
                    // No specific UI buffer to clear for VelocityComponent, unlike ScriptComponent
                }
            }
        }

        if (scene.componentManager->hasComponent<ScriptComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Script Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& scriptComp = scene.componentManager->getComponent<ScriptComponent>(scene.selectedEntity);
                if (scene.inspectorScriptPathBuffer[0] == '\0' || scriptComp.scriptPath != scene.inspectorScriptPathBuffer) {
                    copyToBuffer(scriptComp.scriptPath, scene.inspectorScriptPathBuffer, IM_ARRAYSIZE(scene.inspectorScriptPathBuffer));
                }
                ImGui::Text("Script Path:");
                if (ImGui::InputText("##ScriptPath", scene.inspectorScriptPathBuffer, IM_ARRAYSIZE(scene.inspectorScriptPathBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    scriptComp.scriptPath = scene.inspectorScriptPathBuffer;
                }
                if (ImGui::Button("Browse...##ScriptPath")) {
                    const char* filterPatterns[] = { "*.lua" };
                    const char* filePath = tinyfd_openFileDialog("Select Lua Script", "../assets/scripts/", 1, filterPatterns, "Lua Scripts", 0);
                    if (filePath != NULL) {
                        std::filesystem::path selectedPath = filePath;
                        scriptComp.scriptPath = selectedPath.string();
                        strncpy(scene.inspectorScriptPathBuffer, scriptComp.scriptPath.c_str(), IM_ARRAYSIZE(scene.inspectorScriptPathBuffer) - 1);
                        scene.inspectorScriptPathBuffer[IM_ARRAYSIZE(scene.inspectorScriptPathBuffer) - 1] = '\0';
                    }
                }
                 if (ImGui::Button("Remove Script Component")) {
                    scene.componentManager->removeComponent<ScriptComponent>(scene.selectedEntity);
                    scene.inspectorScriptPathBuffer[0] = '\0';
                }
            }
        } 

        ImGui::Separator();

        if (scene.componentManager->hasComponent<ColliderComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Collider Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& collider = scene.componentManager->getComponent<ColliderComponent>(scene.selectedEntity);
                ImGui::DragFloat("Offset X##Collider", &collider.offsetX, 0.1f);
                ImGui::DragFloat("Offset Y##Collider", &collider.offsetY, 0.1f);
                ImGui::DragFloat("Width##Collider", &collider.width, 1.0f, 1.0f);
                ImGui::DragFloat("Height##Collider", &collider.height, 1.0f, 1.0f);
                ImGui::Checkbox("Is Trigger##Collider", &collider.isTrigger);

                ImGui::Separator();
                ImGui::Text("Polygon Vertices:");
                int removeIndex = -1;
                for (size_t i = 0; i < collider.vertices.size(); ++i) {
                    ImGui::PushID(static_cast<int>(i));
                    ImGui::DragFloat2("Vertex", &collider.vertices[i].x, 0.5f);
                    ImGui::SameLine();
                    if (ImGui::Button("Remove##Vertex")) {
                        removeIndex = static_cast<int>(i);
                    }
                    ImGui::PopID();
                }
                if (removeIndex >= 0 && removeIndex < (int)collider.vertices.size()) {
                    collider.vertices.erase(collider.vertices.begin() + removeIndex);
                }
                if (ImGui::Button("Add Vertex")) {
                    collider.vertices.push_back({0.0f, 0.0f});
                }
                if (ImGui::Button("Clear Vertices")) {
                    collider.vertices.clear();
                }

                if (ImGui::Button("Remove Collider Component")) {
                    scene.componentManager->removeComponent<ColliderComponent>(scene.selectedEntity);
                    Signature sig = scene.entityManager->getSignature(scene.selectedEntity);
                    sig.reset(scene.componentManager->getComponentType<ColliderComponent>());
                    scene.entityManager->setSignature(scene.selectedEntity, sig);
                    scene.systemManager->entitySignatureChanged(scene.selectedEntity, sig);
                }

                if (ImGui::Checkbox("Edit Collider in Scene", &scene.isEditingCollider)) {
                    if (!scene.isEditingCollider) {
                        scene.isDraggingVertex = false;
                        scene.editingVertexIndex = -1;
                    }
                }
                ImGui::Text("(Click to add, drag to move vertices)");
            }
        } 

        if (scene.componentManager->hasComponent<AnimationComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Animation Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& animComp = scene.componentManager->getComponent<AnimationComponent>(scene.selectedEntity);
                ImGui::InputText("Current Animation##AnimComp", (char*)animComp.currentAnimationName.c_str(), animComp.currentAnimationName.capacity() + 1, ImGuiInputTextFlags_ReadOnly);
                ImGui::InputInt("Current Frame Index##AnimComp", &animComp.currentFrameIndex);
                ImGui::InputFloat("Current Frame Time##AnimComp", &animComp.currentFrameTime);
                ImGui::Checkbox("Is Playing##AnimComp", &animComp.isPlaying);
                ImGui::Checkbox("Flip Horizontal##AnimComp", &animComp.flipHorizontal);
                ImGui::Checkbox("Flip Vertical##AnimComp", &animComp.flipVertical);
                if (ImGui::Button("Browse Animation JSON...##AnimComp")) {
                    const char* filterPatterns[] = { "*.json" };
                    const char* filePath = tinyfd_openFileDialog("Select Animation JSON", "../assets/animations/", 1, filterPatterns, "Animation JSON", 0);
                    if (filePath != NULL) {
                        std::ifstream in(filePath);
                        if (in) {
                            nlohmann::json animJson;
                            in >> animJson;
                            AnimationComponent newAnimComp = animJson.get<AnimationComponent>();
                            animComp = newAnimComp;
                        }
                    }
                }
                // TODO: Add UI for editing animations map - this is complex
            }
        } 

        if (scene.componentManager->hasComponent<AudioComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Audio Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& audioComp = scene.componentManager->getComponent<AudioComponent>(scene.selectedEntity);
                char audioIdBuffer[256];
                strncpy(audioIdBuffer, audioComp.audioId.c_str(), sizeof(audioIdBuffer) - 1);
                audioIdBuffer[sizeof(audioIdBuffer) - 1] = '\0';
                if (ImGui::InputText("Audio ID", audioIdBuffer, sizeof(audioIdBuffer))) {
                    audioComp.audioId = audioIdBuffer;
                }
                ImGui::Checkbox("Is Music", &audioComp.isMusic);
                ImGui::Checkbox("Play On Start", &audioComp.playOnStart);
                ImGui::Checkbox("Loop", &audioComp.loop);
                ImGui::SliderInt("Volume", &audioComp.volume, 0, 128);
                if (ImGui::Button("Remove Audio Component")) {
                    scene.componentManager->removeComponent<AudioComponent>(scene.selectedEntity);
                    Signature sig = scene.entityManager->getSignature(scene.selectedEntity);
                    sig.reset(scene.componentManager->getComponentType<AudioComponent>());
                    scene.entityManager->setSignature(scene.selectedEntity, sig);
                    scene.systemManager->entitySignatureChanged(scene.selectedEntity, sig);
                }

                // Audio asset browser for AudioComponent
                if (ImGui::Button("Browse Audio...##AudioComponent")) {
                    const char* filterPatterns[] = { "*.mp3", "*.wav", "*.ogg", "*.flac" };
                    const char* filePath = tinyfd_openFileDialog("Select Audio File", "../assets/Audio/", 4, filterPatterns, "Audio Files", 0);
                    if (filePath != NULL) {
                        std::filesystem::path selectedPath = filePath;
                        std::string filename = selectedPath.filename().string();
                        std::string assetId = selectedPath.stem().string();
                        std::filesystem::path destDir = std::filesystem::absolute("../assets/Audio/");
                        std::filesystem::path destPath = destDir / filename;
                        try {
                            std::filesystem::create_directories(destDir);
                            std::filesystem::copy_file(selectedPath, destPath, std::filesystem::copy_options::overwrite_existing);
                            AssetManager& assets = AssetManager::getInstance();
                            if (assets.loadMusic(assetId, destPath.string()) || assets.loadSound(assetId, destPath.string())) {
                                audioComp.audioId = assetId;
                            } else {
                                tinyfd_messageBox("Error", "Failed to load audio into AssetManager.", "ok", "error", 1);
                            }
                        } catch (const std::filesystem::filesystem_error& e) {
                            std::cerr << "Filesystem Error: " << e.what() << std::endl;
                            tinyfd_messageBox("Error", ("Failed to copy file: " + std::string(e.what())).c_str(), "ok", "error", 1);
                        }
                    }
                }
            }
        }

        if (scene.componentManager->hasComponent<SoundEffectsComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Sound Effects Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& soundEffectsComp = scene.componentManager->getComponent<SoundEffectsComponent>(scene.selectedEntity);

                ImGui::SliderInt("Default Volume", &soundEffectsComp.defaultVolume, 0, 128);

                ImGui::Separator();
                ImGui::Text("Sound Effects:");

                // Display existing sound effects
                static std::string toRemove = "";
                for (auto& [actionName, audioId] : soundEffectsComp.soundEffects) {
                    ImGui::PushID(actionName.c_str());
                    ImGui::Text("%s: %s", actionName.c_str(), audioId.c_str());
                    ImGui::SameLine();
                    if (ImGui::Button("Test")) {
                        soundEffectsComp.playSound(actionName);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Remove")) {
                        toRemove = actionName;
                    }
                    ImGui::PopID();
                }

                // Remove sound effect if requested
                if (!toRemove.empty()) {
                    soundEffectsComp.removeSoundEffect(toRemove);
                    toRemove = "";
                }

                ImGui::Separator();

                // Add new sound effect
                static char newActionName[64] = "";
                static char newAudioId[64] = "";

                ImGui::InputText("Action Name##SoundEffects", newActionName, sizeof(newActionName));
                ImGui::InputText("Audio ID##SoundEffects", newAudioId, sizeof(newAudioId));

                if (ImGui::Button("Add Sound Effect") && strlen(newActionName) > 0 && strlen(newAudioId) > 0) {
                    soundEffectsComp.addSoundEffect(std::string(newActionName), std::string(newAudioId));
                    newActionName[0] = '\0';
                    newAudioId[0] = '\0';
                }

                ImGui::SameLine();
                if (ImGui::Button("Browse Audio...##SoundEffects")) {
                    const char* filterPatterns[] = { "*.mp3", "*.wav", "*.ogg", "*.flac" };
                    const char* filePath = tinyfd_openFileDialog("Select Audio File", "../assets/Audio/", 4, filterPatterns, "Audio Files", 0);
                    if (filePath != NULL) {
                        std::filesystem::path selectedPath = filePath;
                        std::string filename = selectedPath.filename().string();
                        std::string assetId = selectedPath.stem().string();
                        std::filesystem::path destDir = std::filesystem::absolute("../assets/Audio/");
                        std::filesystem::path destPath = destDir / filename;
                        try {
                            std::filesystem::create_directories(destDir);
                            std::filesystem::copy_file(selectedPath, destPath, std::filesystem::copy_options::overwrite_existing);
                            AssetManager& assets = AssetManager::getInstance();
                            if (assets.loadSound(assetId, destPath.string())) {
                                strncpy(newAudioId, assetId.c_str(), sizeof(newAudioId) - 1);
                                newAudioId[sizeof(newAudioId) - 1] = '\0';
                            } else {
                                tinyfd_messageBox("Error", "Failed to load audio into AssetManager.", "ok", "error", 1);
                            }
                        } catch (const std::filesystem::filesystem_error& e) {
                            std::cerr << "Filesystem Error: " << e.what() << std::endl;
                            tinyfd_messageBox("Error", ("Failed to copy file: " + std::string(e.what())).c_str(), "ok", "error", 1);
                        }
                    }
                }

                if (ImGui::Button("Remove Sound Effects Component")) {
                    scene.componentManager->removeComponent<SoundEffectsComponent>(scene.selectedEntity);
                    Signature sig = scene.entityManager->getSignature(scene.selectedEntity);
                    sig.reset(scene.componentManager->getComponentType<SoundEffectsComponent>());
                    scene.entityManager->setSignature(scene.selectedEntity, sig);
                    scene.systemManager->entitySignatureChanged(scene.selectedEntity, sig);
                }
            }
        }

        if (scene.componentManager->hasComponent<CameraComponent>(scene.selectedEntity)) {
            if (ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& cameraComp = scene.componentManager->getComponent<CameraComponent>(scene.selectedEntity);
                ImGui::DragFloat("Width##Camera", &cameraComp.width, 1.0f, 1.0f, 10000.0f);
                ImGui::DragFloat("Height##Camera", &cameraComp.height, 1.0f, 1.0f, 10000.0f);
                ImGui::DragFloat("Zoom##Camera", &cameraComp.zoom, 0.01f, 0.01f, 100.0f); // Min zoom 0.01, max 100
                
                bool isActive = cameraComp.isActive;
                if (ImGui::Checkbox("Is Active Camera##Camera", &isActive)) {
                    if (isActive) {
                        // Deactivate all other cameras if this one is being set to active
                        for (Entity entity : scene.entityManager->getActiveEntities()) {
                            if (scene.componentManager->hasComponent<CameraComponent>(entity) && entity != scene.selectedEntity) {
                                scene.componentManager->getComponent<CameraComponent>(entity).isActive = false;
                            }
                        }
                    }
                    cameraComp.isActive = isActive;
                }

                ImGui::Checkbox("Lock X##Camera", &cameraComp.lockX);
                ImGui::Checkbox("Lock Y##Camera", &cameraComp.lockY);

                if (ImGui::Button("Remove Camera Component")) {
                    bool wasActive = cameraComp.isActive;
                    scene.componentManager->removeComponent<CameraComponent>(scene.selectedEntity);
                    Signature sig = scene.entityManager->getSignature(scene.selectedEntity);
                    sig.reset(scene.componentManager->getComponentType<CameraComponent>());
                    scene.entityManager->setSignature(scene.selectedEntity, sig);
                    scene.systemManager->entitySignatureChanged(scene.selectedEntity, sig);
                    // If the removed camera was active, try to find another one to activate or log
                    if (wasActive) {
                        bool foundAnotherActive = false;
                        for (Entity entity : scene.entityManager->getActiveEntities()) {
                            if (scene.componentManager->hasComponent<CameraComponent>(entity)) {
                                scene.componentManager->getComponent<CameraComponent>(entity).isActive = true; // Activate the first one found
                                foundAnotherActive = true;
                                std::cout << "Activated another camera (Entity " << entity << ") as fallback." << std::endl;
                                break;
                            }
                        }
                        if (!foundAnotherActive) {
                            std::cout << "Removed active camera. No other cameras found to activate." << std::endl;
                        }
                    }
                }
            }
        }

        // RigidbodyComponent Inspector
        if (scene.componentManager->hasComponent<RigidbodyComponent>(scene.selectedEntity)) {
            auto& rigidbody = scene.componentManager->getComponent<RigidbodyComponent>(scene.selectedEntity);
            ImGui::Separator();
            ImGui::Text("RigidbodyComponent");
            ImGui::InputFloat("Mass", &rigidbody.mass);
            ImGui::Checkbox("Use Gravity", &rigidbody.useGravity);
            ImGui::Checkbox("Is Static", &rigidbody.isStatic);
            ImGui::InputFloat("Gravity Scale", &rigidbody.gravityScale);
            ImGui::InputFloat("Drag", &rigidbody.drag);
            ImGui::Checkbox("Is Kinematic", &rigidbody.isKinematic);
        }
    } else {
        ImGui::Text("No entity selected.");
    }
    ImGui::End();
}

}
