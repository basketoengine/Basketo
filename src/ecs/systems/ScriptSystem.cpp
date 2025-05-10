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
}
