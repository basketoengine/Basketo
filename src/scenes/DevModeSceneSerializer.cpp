\
#include "DevModeSceneSerializer.h"
#include "DevModeScene.h"
#include "../../vendor/nlohmann/json.hpp"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/VelocityComponent.h"
#include "../ecs/components/ScriptComponent.h"
#include "../ecs/components/ColliderComponent.h"
#include "../ecs/components/AnimationComponent.h"
#include "../ecs/components/NameComponent.h"
#include "../ecs/components/AudioComponent.h"
#include "../ecs/components/CameraComponent.h" 
#include "../AssetManager.h"
#include "tinyfiledialogs.h"
#include <fstream>
#include <iostream>
#include <set>

void saveDevModeScene(DevModeScene& scene, const std::string& filepath) {
    nlohmann::json sceneJson;
    sceneJson["entities"] = nlohmann::json::array();

    std::cout << "Saving scene to " << filepath << "..." << std::endl;

    const auto& activeEntities = scene.entityManager->getActiveEntities();
    for (Entity entity : activeEntities) {
        nlohmann::json entityJson;
        entityJson["id_saved"] = entity; 
        entityJson["components"] = nlohmann::json::object();

        if (scene.componentManager->hasComponent<TransformComponent>(entity)) {
            entityJson["components"]["TransformComponent"] = scene.componentManager->getComponent<TransformComponent>(entity);
        }
        if (scene.componentManager->hasComponent<SpriteComponent>(entity)) {
            entityJson["components"]["SpriteComponent"] = scene.componentManager->getComponent<SpriteComponent>(entity);
        }
        if (scene.componentManager->hasComponent<VelocityComponent>(entity)) {
            entityJson["components"]["VelocityComponent"] = scene.componentManager->getComponent<VelocityComponent>(entity);
        }
        if (scene.componentManager->hasComponent<ScriptComponent>(entity)) {
            entityJson["components"]["ScriptComponent"] = scene.componentManager->getComponent<ScriptComponent>(entity);
        }
        if (scene.componentManager->hasComponent<ColliderComponent>(entity)) {
            entityJson["components"]["ColliderComponent"] = scene.componentManager->getComponent<ColliderComponent>(entity);
        }
        if (scene.componentManager->hasComponent<AnimationComponent>(entity)) {
            entityJson["components"]["AnimationComponent"] = scene.componentManager->getComponent<AnimationComponent>(entity);
        }
        if (scene.componentManager->hasComponent<NameComponent>(entity)) {
            entityJson["components"]["NameComponent"] = scene.componentManager->getComponent<NameComponent>(entity);
        }
        if (scene.componentManager->hasComponent<AudioComponent>(entity)) {
            entityJson["components"]["AudioComponent"] = scene.componentManager->getComponent<AudioComponent>(entity);
        }
        if (scene.componentManager->hasComponent<CameraComponent>(entity)) {
            entityJson["components"]["CameraComponent"] = scene.componentManager->getComponent<CameraComponent>(entity);
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

bool loadDevModeScene(DevModeScene& scene, const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file " << filepath << " for reading!" << std::endl;
        tinyfd_messageBox("Load Error", ("Could not open file: " + filepath).c_str(), "ok", "error", 1);
        return false;
    }

    nlohmann::json sceneJson;
    try {
        inFile >> sceneJson;
        inFile.close();
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Error: Failed to parse scene file " << filepath << ". " << e.what() << std::endl;
        inFile.close();
        tinyfd_messageBox("Load Error", ("Failed to parse scene file: " + filepath + "\n" + e.what()).c_str(), "ok", "error", 1);
        return false;
    }

    std::cout << "Loading scene from " << filepath << "..." << std::endl;

    std::set<Entity> entitiesToDestroy = scene.entityManager->getActiveEntities();
    for (Entity entity : entitiesToDestroy) {
        scene.entityManager->destroyEntity(entity);
    }
    if (!scene.entityManager->getActiveEntities().empty()) {
         std::cerr << "Warning: Not all entities were destroyed during scene load cleanup!" << std::endl;
    }
    scene.selectedEntity = NO_ENTITY_SELECTED; 
    scene.inspectorTextureIdBuffer[0] = '\0';
    scene.inspectorScriptPathBuffer[0] = '\0';
    scene.cameraX = 0.0f; 
    scene.cameraY = 0.0f;
    scene.cameraZoom = 1.0f;

    if (!sceneJson.contains("entities") || !sceneJson["entities"].is_array()) {
        std::cerr << "Error: Scene file " << filepath << " does not contain a valid 'entities' array." << std::endl;
        tinyfd_messageBox("Load Error", ("Scene file " + filepath + " is missing or has an invalid 'entities' array.").c_str(), "ok", "error", 1);
        return false;
    }

    for (const auto& entityJson : sceneJson["entities"]) {
        Entity newEntity = scene.entityManager->createEntity();
        Signature entitySignature; 

        if (!entityJson.contains("components") || !entityJson["components"].is_object()) {
            std::cerr << "Warning: Entity definition missing 'components' object. Skipping." << std::endl;
            scene.entityManager->destroyEntity(newEntity); 
            continue;
        }

        const auto& componentsJson = entityJson["components"];

        for (auto it = componentsJson.begin(); it != componentsJson.end(); ++it) {
            const std::string& componentType = it.key();
            const nlohmann::json& componentData = it.value();

            try {
                if (componentType == "TransformComponent") {
                    TransformComponent comp;
                    from_json(componentData, comp);
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<TransformComponent>());
                } else if (componentType == "SpriteComponent") {
                    SpriteComponent comp;
                    from_json(componentData, comp); 

                    if (!scene.assetManager.getTexture(comp.textureId)) {
                        std::cout << "LoadScene: Texture '" << comp.textureId << "' not pre-loaded. Attempting dynamic load..." << std::endl;
                        
                        std::string potentialPath = "../assets/Textures/" + comp.textureId + ".png";
                        if (!scene.assetManager.loadTexture(comp.textureId, potentialPath)) {
                            potentialPath = "../assets/Textures/" + comp.textureId + ".jpg"; 
                             if (!scene.assetManager.loadTexture(comp.textureId, potentialPath)) {
                                std::cerr << "LoadScene Warning: Failed to dynamically load texture '" << comp.textureId 
                                          << "' needed by loaded entity " << newEntity << ". Check path/extension." << std::endl;
                            } else {
                                std::cout << "LoadScene: Successfully loaded texture '" << comp.textureId << "' dynamically (as .jpg)." << std::endl;
                            }
                        } else {
                             std::cout << "LoadScene: Successfully loaded texture '" << comp.textureId << "' dynamically (as .png)." << std::endl;
                        }
                    }
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<SpriteComponent>());
                } else if (componentType == "VelocityComponent") {
                    VelocityComponent comp;
                    from_json(componentData, comp);
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<VelocityComponent>());
                } else if (componentType == "ScriptComponent") {
                    ScriptComponent comp;
                    from_json(componentData, comp);
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<ScriptComponent>());
                } else if (componentType == "ColliderComponent") {
                    ColliderComponent comp;
                    from_json(componentData, comp); 
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<ColliderComponent>());
                } else if (componentType == "AnimationComponent") { // Add AnimationComponent deserialization
                    AnimationComponent comp;
                    from_json(componentData, comp);
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<AnimationComponent>());
                } else if (componentType == "NameComponent") {
                    NameComponent comp;
                    from_json(componentData, comp);
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<NameComponent>());
                } else if (componentType == "AudioComponent") {
                    AudioComponent comp;
                    from_json(componentData, comp);
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<AudioComponent>());
                } else if (componentType == "CameraComponent") { // Deserialize CameraComponent
                    CameraComponent comp;
                    from_json(componentData, comp);
                    scene.componentManager->addComponent(newEntity, comp);
                    entitySignature.set(scene.componentManager->getComponentType<CameraComponent>());
                } else {
                    std::cerr << "Warning: Unknown component type '" << componentType << "' encountered during loading." << std::endl;
                }
            } catch (nlohmann::json::exception& e) {
                std::cerr << "Error parsing component '" << componentType << "' for entity. " << e.what() << std::endl;
            }
        }
        scene.entityManager->setSignature(newEntity, entitySignature);
        
    }
    std::cout << "Scene loaded successfully from " << filepath << std::endl;
    return true;
}
