#pragma once

#include <string>
#include <unordered_map>
#include "../System.h"
#include "../Entity.h"
#include <sol/sol.hpp> // Changed to use angle brackets

// Forward declarations
class EntityManager;
class ComponentManager;

class ScriptSystem : public System {
public:
    ScriptSystem(EntityManager* entityManager, ComponentManager* componentManager);
    ~ScriptSystem();

    bool init(); // Initialize Lua state and register base functions
    void update(float deltaTime); // Call onUpdate for all scripted entities

    // Load a script for an entity. Replaces existing script.
    bool loadScript(Entity entity, const std::string& scriptPath);

    // Call a specific function in a script's environment
    template<typename... Args>
    sol::protected_function_result callScriptFunction(Entity entity, const std::string& functionName, Args&&... args);

    // Expose a C++ function to Lua globally
    template<typename Func>
    void registerFunction(const std::string& luaName, Func&& f);
    
    // Get the Lua state
    sol::state& getLuaState() { return lua; }

private:
    EntityManager* entityManager;
    ComponentManager* componentManager;
    sol::state lua;

    // Stores the environment for each entity's script to keep them isolated
    std::unordered_map<Entity, sol::environment> entityScriptEnvironments;

    void registerCoreAPI(); // Register engine core functionalities
    void registerEntityAPI(); // Register entity-related functions
};

// Template implementation for registerFunction
template<typename Func>
void ScriptSystem::registerFunction(const std::string& luaName, Func&& f) {
    lua.set_function(luaName, std::forward<Func>(f));
}

// Template implementation for callScriptFunction
template<typename... Args>
sol::protected_function_result ScriptSystem::callScriptFunction(Entity entity, const std::string& functionName, Args&&... args) {
    auto it = entityScriptEnvironments.find(entity);
    if (it != entityScriptEnvironments.end()) {
        sol::environment& env = it->second;
        sol::protected_function func = env[functionName];
        if (func.valid()) {
            return func(std::forward<Args>(args)...);
        }
    }
    sol::protected_function_result result; // Default-constructs to an invalid, error state
    return result;
}
