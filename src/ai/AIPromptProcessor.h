\
#ifndef AIPROMPTPROCESSOR_H
#define AIPROMPTPROCESSOR_H

#include <string>
#include <functional>
#include <future>
#include <deque>
#include <mutex>
#include <vector>
#include <chrono>
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

    // Agent mode functionality
    void enableAgentMode(bool enable = true);
    bool isAgentModeEnabled() const;
    void pauseAgentMode(bool pause = true);
    bool isAgentModePaused() const;
    void addAgentTask(const std::string& task);
    void clearAgentTasks();
    void processNextAgentTask();
    void updateAgentMode();

    // API Key management
    void setApiKey(const std::string& apiKey);
    bool isApiKeyConfigured() const;
    void saveApiKeyToConfig(const std::string& apiKey);

private:
    std::string ProcessGeminiPrompt(const std::string& promptText);
    void GenerateScriptFromGemini(const std::string& scriptPrompt, std::string& outScriptPath);
    void ModifyComponentFromGemini(Entity entity, const std::string& modificationPrompt);
    std::string TranslateNaturalLanguageToCommand(const std::string& naturalLanguageInput);
    std::string ListAssets();
    std::string BuildComprehensiveContext();
    std::string ExtractGeminiResponseText(const std::string& jsonResponse);

    EntityManager* m_entityManager;
    ComponentManager* m_componentManager;
    SystemManager* m_systemManager;
    AssetManager* m_assetManager;
    std::function<Entity(const std::string&)> m_findEntityByNameFunc;
    char m_llmPromptBuffer[1024];
    char m_apiKeyBuffer[512];  // Buffer for API key input
    std::string m_apiKey;
    bool m_showApiKeyInput = false;  // Toggle for showing API key input section

    // Missing member variables
    bool m_isProcessing = false;
    std::string m_lastApiResponse;

    // Agent mode functionality
    bool m_agentMode = false;
    std::deque<std::string> m_agentTaskQueue;
    std::vector<std::string> m_conversationHistory;
    std::string m_agentContext;
    int m_maxAgentTasks = 10;
    bool m_agentPaused = false;
    std::chrono::steady_clock::time_point m_lastAgentActivity;

    std::future<std::string> m_geminiFuture;

};

#endif
