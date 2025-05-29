\
#ifndef AIPROMPTPROCESSOR_H
#define AIPROMPTPROCESSOR_H

#include <string>
#include <functional>
#include <future> 
#include <deque>
#include <mutex> 
#include "../ecs/Entity.h" 
const Entity NO_ENTITY_SELECTED = MAX_ENTITIES; 

class EntityManager;
class ComponentManager;
class SystemManager;
class AssetManager;

class AIPromptProcessor {
public:
    AIPromptProcessor(
        EntityManager* entityManager,
        ComponentManager* componentManager,
        SystemManager* systemManager,
        AssetManager* assetManager,
        std::function<Entity(const std::string&)> findEntityByNameFunc
    );

    void renderAIPromptUI();
    void HandlePrompt(const std::string& prompt, bool isTranslatedCommand = false);
    void PollAndProcessPendingCommands();

private:
    std::string ProcessGeminiPrompt(const std::string& promptText);
    void GenerateScriptFromGemini(const std::string& scriptPrompt, std::string& outScriptPath);
    void ModifyComponentFromGemini(Entity entity, const std::string& modificationPrompt); 
    std::string TranslateNaturalLanguageToCommand(const std::string& naturalLanguageInput);

    EntityManager* m_entityManager;
    ComponentManager* m_componentManager;
    SystemManager* m_systemManager;
    AssetManager* m_assetManager;
    std::function<Entity(const std::string&)> m_findEntityByNameFunc;
    char m_llmPromptBuffer[1024];
    std::string m_apiKey; 

    std::future<std::string> m_geminiFuture;

};

#endif=
