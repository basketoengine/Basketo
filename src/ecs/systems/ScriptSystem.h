#pragma once

#include <string>
#include <unordered_map>
#include "../System.h"
#include "../Entity.h"
#include <sol/sol.hpp> 
#include <functional> 

class EntityManager;
class ComponentManager;

class ScriptSystem : public System {
public:
    ScriptSystem(EntityManager* entityManager, ComponentManager* componentManager);
    ~ScriptSystem();

    bool init();
    void setLoggingFunctions(std::function<void(const std::string&)> logFunc, std::function<void(const std::string&)> errorFunc);
    void update(float deltaTime);

    bool loadScript(Entity entity, const std::string& scriptPath);

    template<typename... Args>
    sol::protected_function_result callScriptFunction(Entity entity, const std::string& functionName, Args&&... args);

    template<typename Func>
    void registerFunction(const std::string& luaName, Func&& f);
    
    sol::state& getLuaState() { return lua; }

private:
    EntityManager* entityManager;
    ComponentManager* componentManager;
    sol::state lua;
    std::function<void(const std::string&)> logCallback;
    std::function<void(const std::string&)> errorLogCallback;

    std::unordered_map<Entity, sol::environment> entityScriptEnvironments;

    void registerCoreAPI(); 
    void registerEntityAPI(); 
};

template<typename Func>
void ScriptSystem::registerFunction(const std::string& luaName, Func&& f) {
    lua.set_function(luaName, std::forward<Func>(f));
}

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
    sol::protected_function_result result; 
    return result;
}
