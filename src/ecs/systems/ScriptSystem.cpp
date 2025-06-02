#include "ScriptSystem.h"
#include "../EntityManager.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/ScriptComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/RigidbodyComponent.h"
#include "../components/AnimationComponent.h"
#include "../components/ColliderComponent.h"
#include "../components/AudioComponent.h"
#include "../../InputManager.h"
#include <iostream>
#include <fstream>

ScriptSystem::ScriptSystem(EntityManager* em, ComponentManager* cm)
    : entityManager(em), componentManager(cm) {}

ScriptSystem::~ScriptSystem() {}

void ScriptSystem::setLoggingFunctions(std::function<void(const std::string&)> logFunc, std::function<void(const std::string&)> errorFunc) {
    logCallback = logFunc;
    errorLogCallback = errorFunc;
}

bool ScriptSystem::init() {
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::io);
    std::cout << "ScriptSystem: Lua initialized." << std::endl;
    registerCoreAPI();
    registerEntityAPI();
    return true;
}

void ScriptSystem::update(float deltaTime) {
    std::cout << "[ScriptSystem] C++ update method CALLED. DeltaTime: " << deltaTime << std::endl; // ADDED THIS LOG
    // Log("[ScriptSystem] Update called. Processing entities..."); // Optional: General system update log
    for (auto entity : entityManager->getActiveEntities()) {
        // Log("[ScriptSystem] Checking entity: " + std::to_string(entity)); // Optional: Log each entity being checked
        if (componentManager->hasComponent<ScriptComponent>(entity)) {
            auto& scriptComp = componentManager->getComponent<ScriptComponent>(entity);
            std::cout << "[ScriptSystem] Entity " << entity << " has ScriptComponent with path: '" << scriptComp.scriptPath << "'" << std::endl; // Log 1
            if (!scriptComp.scriptPath.empty()) {
                // Check if the entity has a registered script environment
                if (entityScriptEnvironments.count(entity)) {
                    std::cout << "[ScriptSystem] Entity " << entity << " has a script environment. Attempting to call 'update'." << std::endl; // Log 2
                    callScriptFunction(entity, "update", entity, deltaTime);
                } else {
                    std::cerr << "[ScriptSystem] ERROR: Entity " << entity << " has ScriptComponent but NO script environment registered. Cannot call 'update'." << std::endl; // Log 3
                }
            } else {
                 std::cout << "[ScriptSystem] Entity " << entity << " has ScriptComponent but scriptPath is EMPTY." << std::endl; // Log 4
            }
        }
    }
}

bool ScriptSystem::loadScript(Entity entity, const std::string& scriptPath) {
    if (!componentManager->hasComponent<ScriptComponent>(entity)) {
        std::cerr << "ScriptSystem Error: Entity " << entity << " does not have a ScriptComponent. Cannot load script." << std::endl;
        return false;
    }

    std::ifstream scriptFile(scriptPath);
    if (!scriptFile.is_open()) {
        std::cerr << "ScriptSystem Error: Could not open script file: " << scriptPath << std::endl;
        return false;
    }
    scriptFile.close();

    sol::environment env(lua, sol::create, lua.globals());
    entityScriptEnvironments[entity] = env;

    try {
        auto result = lua.safe_script_file(scriptPath, entityScriptEnvironments[entity]);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "ScriptSystem Error: Failed to load or execute script '" << scriptPath << "' for entity " << entity << ": " << err.what() << std::endl;
            entityScriptEnvironments.erase(entity);
            return false;
        }
        std::cout << "ScriptSystem: Successfully loaded script '" << scriptPath << "' for entity " << entity << std::endl;

        sol::protected_function_result initResult = callScriptFunction(entity, "init", entity);
        if (!initResult.valid()) {
            sol::error err = initResult;
        }

    } catch (const sol::error& e) {
        std::cerr << "ScriptSystem Sol2 Error: Exception during script load for entity " << entity << " with script '" << scriptPath << "': " << e.what() << std::endl;
        entityScriptEnvironments.erase(entity);
        return false;
    }

    return true;
}


void ScriptSystem::registerCoreAPI() {
    lua.new_usertype<InputManager>("Input",
        sol::no_constructor,
        "isKeyDown", [](const std::string& keyName) { 
            SDL_Scancode scancode = SDL_GetScancodeFromName(keyName.c_str());
            if (scancode != SDL_SCANCODE_UNKNOWN) {
                const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
                return keyboardState && keyboardState[scancode];
            }
            if (keyName == "W") return InputManager::getInstance().isActionPressed("MoveUp");
            if (keyName == "A") return InputManager::getInstance().isActionPressed("MoveLeft");
            if (keyName == "S") return InputManager::getInstance().isActionPressed("MoveDown");
            if (keyName == "D") return InputManager::getInstance().isActionPressed("MoveRight");
            return false; 
        }
    );
    lua["Input"] = &InputManager::getInstance(); 

    registerFunction("Log", [this](const std::string& message) {
        if (logCallback) {
            logCallback("[LUA] " + message);
        } else {
            std::cout << "[LUA] " << message << std::endl;
        }
    });
    registerFunction("LogError", [this](const std::string& message) {
        if (errorLogCallback) {
            errorLogCallback("[LUA ERROR] " + message);
        } else {
            std::cerr << "[LUA ERROR] " << message << std::endl;
        }
    });
}

void ScriptSystem::registerEntityAPI() {
    lua.new_usertype<Entity>("Entity", sol::constructors<Entity(unsigned int)>());


    registerFunction("GetEntityPosition", [this](Entity entity) -> sol::object {
        if (componentManager->hasComponent<TransformComponent>(entity)) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            return sol::make_object(lua, sol::as_table(std::vector<float>{transform.x, transform.y}));
        }
        return sol::nil;
    });

    registerFunction("SetEntityPosition", [this](Entity entity, float x, float y) {
        if (componentManager->hasComponent<TransformComponent>(entity)) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            transform.x = x;
            transform.y = y;
        }
    });

    registerFunction("MoveEntity", [this](Entity entity, float dx, float dy) {
        if (componentManager->hasComponent<TransformComponent>(entity)) {
            auto& transform = componentManager->getComponent<TransformComponent>(entity);
            transform.x += dx;
            transform.y += dy;
        }
    });

    registerFunction("SetEntityVelocity", [this](Entity entity, float vx, float vy) {
        if (!componentManager->hasComponent<VelocityComponent>(entity)) {
            std::cerr << "[LUA ERROR] SetEntityVelocity: Entity " << entity << " does not have a VelocityComponent. Cannot set velocity." << std::endl;
            return;
        }
        if (componentManager->hasComponent<VelocityComponent>(entity)) { 
            auto& velocity = componentManager->getComponent<VelocityComponent>(entity);
            velocity.vx = vx;
            velocity.vy = vy;
        }
    });

    registerFunction("GetEntityVelocity", [this](Entity entity) -> sol::object {
        if (componentManager->hasComponent<VelocityComponent>(entity)) {
            auto& velocity = componentManager->getComponent<VelocityComponent>(entity);
            return sol::make_object(lua, sol::as_table(std::vector<float>{velocity.vx, velocity.vy}));
        }
        std::cerr << "[LUA ERROR] GetEntityVelocity: Entity " << entity << " does not have a VelocityComponent." << std::endl;
        return sol::nil;
    });

    registerFunction("IsEntityGrounded", [this](Entity entity) -> bool {
        if (componentManager->hasComponent<RigidbodyComponent>(entity)) {
            auto& rigidbody = componentManager->getComponent<RigidbodyComponent>(entity);
            // return rigidbody.isGrounded; // Removed direct isGrounded check
            // This function is now deprecated in favor of GetCollisionContacts
            if (errorLogCallback) errorLogCallback("[LUA WARNING] IsEntityGrounded is deprecated. Use GetCollisionContacts instead.");
        }
        // std::cerr << "[LUA ERROR] IsEntityGrounded: Entity " << entity << " does not have a RigidbodyComponent." << std::endl;
        // if (errorLogCallback) errorLogCallback("[LUA ERROR] IsEntityGrounded: Entity " + std::to_string(entity) + " does not have a RigidbodyComponent.");
        return false; 
    });

    registerFunction("SetEntityAnimation", [this](Entity entity, const std::string& animationName, sol::optional<bool> forceRestartOpt) {
        if (componentManager->hasComponent<AnimationComponent>(entity)) {
            auto& animComp = componentManager->getComponent<AnimationComponent>(entity);
            bool forceRestart = forceRestartOpt.value_or(false); // Default to false if not provided
            if (!animComp.play(animationName, forceRestart)) {
                if (errorLogCallback) {
                    errorLogCallback("[LUA ERROR] SetEntityAnimation: Animation '" + animationName + "' not found or failed to play for entity " + std::to_string(entity));
                }
            }
        } else {
            if (errorLogCallback) {
                errorLogCallback("[LUA ERROR] SetEntityAnimation: Entity " + std::to_string(entity) + " does not have an AnimationComponent.");
            }
        }
    });

    registerFunction("GetEntityAnimation", [this](Entity entity) -> sol::object {
        if (componentManager->hasComponent<AnimationComponent>(entity)) {
            auto& animComp = componentManager->getComponent<AnimationComponent>(entity);
            return sol::make_object(lua, animComp.currentAnimationName);
        } else {
            if (errorLogCallback) {
                errorLogCallback("[LUA ERROR] GetEntityAnimation: Entity " + std::to_string(entity) + " does not have an AnimationComponent.");
            }
        }
        return sol::nil;
    });

    registerFunction("IsAnimationPlaying", [this](Entity entity, sol::optional<std::string> animationNameOpt) -> bool {
        if (componentManager->hasComponent<AnimationComponent>(entity)) {
            auto& animComp = componentManager->getComponent<AnimationComponent>(entity);
            if (animationNameOpt.has_value()) {
                return animComp.isPlaying && animComp.currentAnimationName == animationNameOpt.value();
            }
            return animComp.isPlaying;
        } else {
            if (errorLogCallback) {
                errorLogCallback("[LUA ERROR] IsAnimationPlaying: Entity " + std::to_string(entity) + " does not have an AnimationComponent.");
            }
        }
        return false;
    });

    registerFunction("PlayAnimation", [this](Entity entity, const std::string& animationName) {
        if (componentManager->hasComponent<AnimationComponent>(entity)) {
            auto& animComp = componentManager->getComponent<AnimationComponent>(entity);
            if (!animComp.play(animationName, true)) { 
                 if (errorLogCallback) {
                    errorLogCallback("[LUA ERROR] PlayAnimation: Animation '" + animationName + "' not found or failed to play for entity " + std::to_string(entity));
                }
            }
        } else {
             if (errorLogCallback) {
                errorLogCallback("[LUA ERROR] PlayAnimation: Entity " + std::to_string(entity) + " does not have an AnimationComponent.");
            }
        }
    });

    registerFunction("GetCollisionContacts", [this](Entity entity) -> sol::object {
        if (componentManager->hasComponent<ColliderComponent>(entity)) {
            auto& collider = componentManager->getComponent<ColliderComponent>(entity);
            sol::table contacts_table = lua.create_table();
            int i = 1;
            for (const auto& contact : collider.contacts) {
                sol::table contact_table = lua.create_table();
                contact_table["otherEntity"] = contact.otherEntity;
                contact_table["normalX"] = contact.normal.x;
                contact_table["normalY"] = contact.normal.y;
                contacts_table[i++] = contact_table;
            }
            return contacts_table;
        }
        if (errorLogCallback) {
            errorLogCallback("[LUA ERROR] GetCollisionContacts: Entity " + std::to_string(entity) + " does not have a ColliderComponent.");
        }
        return sol::nil; 
    });

    registerFunction("HasEntityComponent", [this](Entity entity, const std::string& componentName) -> bool {
        if (!componentManager) {
            if (errorLogCallback) errorLogCallback("[LUA ERROR] HasEntityComponent: ComponentManager is null.");
            return false;
        }

        // Add checks for each component type you want to expose
        if (componentName == "TransformComponent") {
            return componentManager->hasComponent<TransformComponent>(entity);
        } else if (componentName == "VelocityComponent") {
            return componentManager->hasComponent<VelocityComponent>(entity);
        } else if (componentName == "SpriteComponent") {
            if (errorLogCallback) errorLogCallback("[LUA WARNING] HasEntityComponent: SpriteComponent check not fully implemented in C++ example.");
            return false; // Placeholder
        } else if (componentName == "AnimationComponent") {
            return componentManager->hasComponent<AnimationComponent>(entity);
        } else if (componentName == "RigidbodyComponent") {
            return componentManager->hasComponent<RigidbodyComponent>(entity);
        } else if (componentName == "ScriptComponent") {
            return componentManager->hasComponent<ScriptComponent>(entity);
        } else if (componentName == "ColliderComponent") {
            return componentManager->hasComponent<ColliderComponent>(entity);
        } else if (componentName == "SoundEffectsComponent") {
            return componentManager->hasComponent<SoundEffectsComponent>(entity);
        } else if (componentName == "AudioComponent") {
            return componentManager->hasComponent<AudioComponent>(entity);
        }
        // Add other components as needed

        if (errorLogCallback) {
            errorLogCallback("[LUA WARNING] HasEntityComponent: Unknown component name '\" + componentName + \"' for entity \" + std::to_string(entity)");
        }
        return false;
    });

    registerFunction("SetEntityFlipHorizontal", [this](Entity entity, bool flip) {
        if (componentManager->hasComponent<AnimationComponent>(entity)) {
            auto& animComp = componentManager->getComponent<AnimationComponent>(entity);
            animComp.flipHorizontal = flip;
        }
    });

    // Sound Effects API
    registerFunction("PlaySound", [this](Entity entity, const std::string& actionName) {
        if (componentManager->hasComponent<SoundEffectsComponent>(entity)) {
            auto& soundEffectsComp = componentManager->getComponent<SoundEffectsComponent>(entity);
            soundEffectsComp.playSound(actionName);
        } else {
            if (errorLogCallback) {
                errorLogCallback("[LUA ERROR] PlaySound: Entity " + std::to_string(entity) + " does not have a SoundEffectsComponent.");
            }
        }
    });

    registerFunction("AddSoundEffect", [this](Entity entity, const std::string& actionName, const std::string& audioId) {
        if (componentManager->hasComponent<SoundEffectsComponent>(entity)) {
            auto& soundEffectsComp = componentManager->getComponent<SoundEffectsComponent>(entity);
            soundEffectsComp.addSoundEffect(actionName, audioId);
        } else {
            if (errorLogCallback) {
                errorLogCallback("[LUA ERROR] AddSoundEffect: Entity " + std::to_string(entity) + " does not have a SoundEffectsComponent.");
            }
        }
    });

    registerFunction("RemoveSoundEffect", [this](Entity entity, const std::string& actionName) {
        if (componentManager->hasComponent<SoundEffectsComponent>(entity)) {
            auto& soundEffectsComp = componentManager->getComponent<SoundEffectsComponent>(entity);
            soundEffectsComp.removeSoundEffect(actionName);
        } else {
            if (errorLogCallback) {
                errorLogCallback("[LUA ERROR] RemoveSoundEffect: Entity " + std::to_string(entity) + " does not have a SoundEffectsComponent.");
            }
        }
    });

    registerFunction("HasSoundEffect", [this](Entity entity, const std::string& actionName) -> bool {
        if (componentManager->hasComponent<SoundEffectsComponent>(entity)) {
            auto& soundEffectsComp = componentManager->getComponent<SoundEffectsComponent>(entity);
            return !soundEffectsComp.getAudioId(actionName).empty();
        } else {
            if (errorLogCallback) {
                errorLogCallback("[LUA ERROR] HasSoundEffect: Entity " + std::to_string(entity) + " does not have a SoundEffectsComponent.");
            }
        }
        return false;
    });
}
