#pragma once

#include "Types.h"
#include "System.h"
#include "ComponentManager.h"
#include <unordered_map>
#include <memory>
#include <set>
#include <iostream>
#include <string> 
#include <typeinfo> 
#include "systems/CollisionSystem.h" 

class SystemManager {
public:
    template<typename T, typename... Args>  
    std::shared_ptr<T> registerSystem(Args&&... args) {
        const char* typeName = typeid(T).name();
        std::cout << "[SystemManager] Attempting to register system: " << typeName << std::endl;

        if (systems.find(typeName) != systems.end()) {
            std::cout << "[SystemManager] System " << typeName << " already registered. Returning existing instance." << std::endl;
            return std::static_pointer_cast<T>(systems[typeName]);
        }

        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        systems.insert({typeName, system});
        std::cout << "[SystemManager] Successfully registered and created system: " << typeName << std::endl;
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
            auto const& systemTypeNameStd = pair.first; 
            auto const& system = pair.second;
            
            std::cout << "[SystemManager Debug] Processing system from map. Key: '" << systemTypeNameStd << "'" << std::endl;
            std::cout << "[SystemManager Debug] Comparing with typeid(CollisionSystem).name(): '" << typeid(CollisionSystem).name() << "'" << std::endl;
            bool namesMatch = (std::string(systemTypeNameStd) == std::string(typeid(CollisionSystem).name()));
            std::cout << "[SystemManager Debug] Do these names match? " << (namesMatch ? "YES" : "NO") << std::endl;

            auto sig_it = signatures.find(systemTypeNameStd);
            if (sig_it != signatures.end()) {
                auto const& systemSignature = sig_it->second;

                if (namesMatch) { 
                    std::cout << "[SystemManager] Checking for CollisionSystem (Entity " << entity << "):" << std::endl;
                    std::cout << "  Entity Current Signature (bits):      " << entitySignature.to_string() << std::endl;
                    std::cout << "  CollisionSystem Required Sig (bits): " << systemSignature.to_string() << std::endl;
                    Signature combined = entitySignature & systemSignature;
                    std::cout << "  EntitySig & SystemReqSig (bits):    " << combined.to_string() << std::endl;
                    bool signatureHolds = (combined == systemSignature);
                    std::cout << "  Condition ((EntitySig & SystemReqSig) == SystemReqSig): " << (signatureHolds ? "MET" : "NOT MET") << std::endl;
                    if (signatureHolds) {
                        if (system->entities.find(entity) == system->entities.end()) {
                            std::cout << "  >>> Entity " << entity << " WILL BE ADDED to CollisionSystem." << std::endl;
                        } else {
                            std::cout << "  >>> Entity " << entity << " ALREADY IN CollisionSystem (no change)." << std::endl;
                        }
                    } else {
                        if (system->entities.find(entity) != system->entities.end()) {
                            std::cout << "  >>> Entity " << entity << " WILL BE REMOVED from CollisionSystem." << std::endl;
                        } else {
                             std::cout << "  >>> Entity " << entity << " REMAINS NOT IN CollisionSystem (no match)." << std::endl;
                        }
                    }
                }

                if ((entitySignature & systemSignature) == systemSignature) {
                    system->entities.insert(entity);
                } else {
                    system->entities.erase(entity);
                }
            } else {
                 std::cout << "[SystemManager Debug] Signature NOT FOUND in 'signatures' map for key: '" << systemTypeNameStd << "'" << std::endl;
            }
        }
    }

private:
    std::unordered_map<const char*, Signature> signatures{};
    std::unordered_map<const char*, std::shared_ptr<System>> systems{};
};
