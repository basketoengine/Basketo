#include "ScriptSystem.h"
#include "../EntityManager.h"
#include "../ComponentManager.h"
#include "../components/TransformComponent.h"
#include "../components/ScriptComponent.h"
#include "../components/VelocityComponent.h"
#include "../../InputManager.h" 
#include <iostream>
#include <fstream>

ScriptSystem::ScriptSystem(EntityManager* em, ComponentManager* cm)
    : entityManager(em), componentManager(cm) {}

ScriptSystem::~ScriptSystem() {}

bool ScriptSystem::init() {
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::io);
    std::cout << "ScriptSystem: Lua initialized." << std::endl;
    registerCoreAPI();
    registerEntityAPI();
    return true;
}

void ScriptSystem::update(float deltaTime) {
    for (auto entity : entityManager->getActiveEntities()) {
        if (componentManager->hasComponent<ScriptComponent>(entity)) {
            auto& scriptComp = componentManager->getComponent<ScriptComponent>(entity);
            if (!scriptComp.scriptPath.empty()) {
                callScriptFunction(entity, "update", entity, deltaTime);
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
    scriptFile.close(); // Just checking if it exists for now, sol will load it.

    // Create a new environment for this script if it doesn't exist
    // or reuse the existing one to allow for hot-reloading (though full hot-reloading needs more)
    sol::environment env(lua, sol::create, lua.globals());
    entityScriptEnvironments[entity] = env;

    try {
        auto result = lua.safe_script_file(scriptPath, entityScriptEnvironments[entity]);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "ScriptSystem Error: Failed to load or execute script '" << scriptPath << "' for entity " << entity << ": " << err.what() << std::endl;
            entityScriptEnvironments.erase(entity); // Clean up failed environment
            return false;
        }
        std::cout << "ScriptSystem: Successfully loaded script '" << scriptPath << "' for entity " << entity << std::endl;

        // Call an optional init function in the script
        sol::protected_function_result initResult = callScriptFunction(entity, "init", entity);
        if (!initResult.valid()) {
            sol::error err = initResult;
            // This might not be an error if the script just doesn't have an init function
            // std::cout << "ScriptSystem Info: No 'init' function or error in 'init' for entity " << entity << ": " << err.what() << std::endl;
        }

    } catch (const sol::error& e) {
        std::cerr << "ScriptSystem Sol2 Error: Exception during script load for entity " << entity << " with script '" << scriptPath << "': " << e.what() << std::endl;
        entityScriptEnvironments.erase(entity); // Clean up failed environment
        return false;
    }

    return true;
}


void ScriptSystem::registerCoreAPI() {
    // Input API Example
    lua.new_usertype<InputManager>("Input",
        sol::no_constructor,
        "isKeyDown", [](const std::string& keyName) { 
            // This is a simplified example. You'd need scancode mapping or similar.
            // For now, let's assume keyName is like "W", "A", etc.
            // This needs to be mapped to SDL_Scancode in your InputManager
            if (keyName == "W") return InputManager::getInstance().isActionPressed("MoveUp"); // Example mapping
            if (keyName == "A") return InputManager::getInstance().isActionPressed("MoveLeft");
            if (keyName == "S") return InputManager::getInstance().isActionPressed("MoveDown");
            if (keyName == "D") return InputManager::getInstance().isActionPressed("MoveRight");
            // Add more key mappings as needed
            return false; 
        }
    );
    // Make InputManager accessible globally via a function or a pre-created instance
    lua["Input"] = &InputManager::getInstance(); 

    // Logging example
    registerFunction("Log", [](const std::string& message) {
        std::cout << "[LUA] " << message << std::endl;
    });
    registerFunction("LogError", [](const std::string& message) {
        std::cerr << "[LUA ERROR] " << message << std::endl;
    });
}

void ScriptSystem::registerEntityAPI() {
    // Entity UserType for Lua
    // This allows passing Entity (which is just an ID) to Lua functions
    // And then using that ID to interact with components from Lua via C++ bridges
    lua.new_usertype<Entity>("Entity", sol::constructors<Entity(unsigned int)>());

    // Example: Get/Set TransformComponent position
    // Note: These functions will be part of the global Lua state but operate on a passed Entity ID.
    // For better organization, you might group them under an "EntityAPI" table in Lua.

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
        if (componentManager->hasComponent<VelocityComponent>(entity)) { // This check is now redundant due to the one above, but harmless
            auto& velocity = componentManager->getComponent<VelocityComponent>(entity);
            velocity.vx = vx;
            velocity.vy = vy;
            // Optional: Log that velocity was set
            // std::cout << "[LUA DEBUG] SetEntityVelocity: Entity " << entity << " velocity set to (" << vx << ", " << vy << ")" << std::endl;
        }
    });

    // Example: Get/Set Velocity (if you add VelocityComponent to Lua)
    // registerFunction("GetEntityVelocity", ...);
    // registerFunction("SetEntityVelocity", ...);

    // You would add more functions here to expose other components or entity operations
    // For example, creating/destroying entities, adding/removing components (more advanced)
}
