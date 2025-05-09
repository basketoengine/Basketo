#pragma once

#include "Types.h"
#include "System.h"
#include "ComponentManager.h"
#include <unordered_map>
#include <memory>
#include <set>

class SystemManager {
public:
    template<typename T, typename... Args>  
    std::shared_ptr<T> registerSystem(Args&&... args) {
        const char* typeName = typeid(T).name();
        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        systems.insert({typeName, system});
        return system;
    }

    template<typename T>
    void setSignature(Signature signature) {
        const char* typeName = typeid(T).name();
        signatures.insert({typeName, signature});
    }

    void entityDestroyed(Entity entity) {
        for (auto const& pair : systems) {
            auto const& system = pair.second;
            system->entities.erase(entity);
        }
    }

    void entitySignatureChanged(Entity entity, Signature entitySignature) {
        for (auto const& pair : systems) {
            auto const& type = pair.first;
            auto const& system = pair.second;
            
            auto sig_it = signatures.find(type);
            if (sig_it != signatures.end()) {
                auto const& systemSignature = sig_it->second;

                if ((entitySignature & systemSignature) == systemSignature) {
                    system->entities.insert(entity);
                } else {
                    system->entities.erase(entity);
                }
            }
            // If no signature is found for a system, it implies it doesn't care about component signatures
            // or it manages its entities differently (e.g., ScriptSystem might iterate all entities with a ScriptComponent directly).
            // Thus, we don't necessarily need an else clause here to remove entities if the signature isn't found.
        }
    }

private:
    std::unordered_map<const char*, Signature> signatures{};
    std::unordered_map<const char*, std::shared_ptr<System>> systems{};
};
