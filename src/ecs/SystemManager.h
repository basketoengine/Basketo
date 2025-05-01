\
#pragma once

#include "Types.h"
#include "System.h"
#include "ComponentManager.h" 
#include <unordered_map>
#include <memory>
#include <set>

class SystemManager {
public:
    template<typename T>
    std::shared_ptr<T> registerSystem() {
        const char* typeName = typeid(T).name();
        // Ensure system isn't already registered
        // assert(systems.find(typeName) == systems.end() && "Registering system more than once.");

        auto system = std::make_shared<T>();
        systems.insert({typeName, system});
        return system;
    }

    template<typename T>
    void setSignature(Signature signature) {
        const char* typeName = typeid(T).name();
        // Ensure system is registered
        // assert(systems.find(typeName) != systems.end() && "System used before registered.");

        signatures.insert({typeName, signature});
    }

    void entityDestroyed(Entity entity) {
        // Erase a destroyed entity from all system lists
        for (auto const& pair : systems) {
            auto const& system = pair.second;
            system->entities.erase(entity);
        }
    }

    void entitySignatureChanged(Entity entity, Signature entitySignature) {
        // Notify each system that an entity's signature changed
        for (auto const& pair : systems) {
            auto const& type = pair.first;
            auto const& system = pair.second;
            auto const& systemSignature = signatures[type];

            // Entity signature matches system signature - insert into set
            if ((entitySignature & systemSignature) == systemSignature) {
                system->entities.insert(entity);
            }
            // Entity signature does not match system signature - erase from set
            else {
                system->entities.erase(entity);
            }
        }
    }

private:
    // Map from system type string pointer to a signature
    std::unordered_map<const char*, Signature> signatures{};

    // Map from system type string pointer to the system instance
    std::unordered_map<const char*, std::shared_ptr<System>> systems{};
};
