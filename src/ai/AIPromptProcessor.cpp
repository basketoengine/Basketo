#include "AIPromptProcessor.h"
#include "imgui.h"
#include <sstream>
#include <string>
#include <vector>
#include <fstream> 
#include <curl/curl.h> 
#include <ctime> 
#include <thread> 
#include <chrono>
#include "../../vendor/nlohmann/json.hpp" // Add this include
#include <filesystem>

#include "../ecs/EntityManager.h"
#include "../ecs/ComponentManager.h"
#include "../ecs/SystemManager.h"
#include "../AssetManager.h"
#include "../ecs/components/NameComponent.h"
#include "../ecs/components/TransformComponent.h"
#include "../ecs/components/SpriteComponent.h"
#include "../ecs/components/ScriptComponent.h"
#include "../ecs/components/RigidbodyComponent.h"
#include "../ecs/components/ColliderComponent.h"
#include "../utils/Console.h" 

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

AIPromptProcessor::AIPromptProcessor(
    EntityManager* entityManager,
    ComponentManager* componentManager,
    SystemManager* systemManager,
    AssetManager* assetManager,
    std::function<Entity(const std::string&)> findEntityByNameFunc
) : m_entityManager(entityManager),
    m_componentManager(componentManager),
    m_systemManager(systemManager),
    m_assetManager(assetManager),
    m_findEntityByNameFunc(findEntityByNameFunc) {
    m_llmPromptBuffer[0] = '\0';
    // Try to load API key from config.json first
    std::string configPath = "config.json";
    if (std::filesystem::exists(configPath)) {
        try {
            std::ifstream configFile(configPath);
            nlohmann::json configJson;
            configFile >> configJson;
            if (configJson.contains("gemini_api_key")) {
                m_apiKey = configJson["gemini_api_key"].get<std::string>();
                Console::Log("Gemini API Key loaded from config.json.");
            }
        } catch (const std::exception& e) {
            Console::Warn(std::string("Failed to read config.json: ") + e.what());
        }
    }
    // Fallback to environment variable if not set in config
    if (m_apiKey.empty()) {
        const char* env_key = std::getenv("GEMINI_API_KEY");
        if (env_key && *env_key) {
            m_apiKey = env_key;
            Console::Log("Gemini API Key loaded from GEMINI_API_KEY environment variable.");
        } else {
            Console::Warn("GEMINI_API_KEY environment variable not set and no key in config.json. API key will be unconfigured.");
        }
    }
}

std::string AIPromptProcessor::ProcessGeminiPrompt(const std::string& prompt) {
    if (m_apiKey.empty()) {
        Console::Error("Gemini API Key is not configured. Please set the GEMINI_API_KEY environment variable.");
        return "error:API Key not configured";
    }
    Console::Log("Processing Gemini prompt: " + prompt);
    CURL* curl = curl_easy_init();
    std::string response_string;

    Console::Log("Initializing cURL for Gemini API request...");
    if (curl) {
        // Use the m_apiKey member variable, which is now loaded by the constructor.
        Console::Log("Using Gemini API Key.");

        std::string url = "https://generativelanguage.googleapis.com/v1/models/gemini-2.0-flash:generateContent?key=" + m_apiKey;
        nlohmann::json payload = {
            {"contents", nlohmann::json::array({
                nlohmann::json{{"parts", nlohmann::json::array({ nlohmann::json{{"text", prompt}} }) }}
            })}
        };
        std::string json_payload = payload.dump();

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        Console::Log("Performing cURL request to Gemini...");
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            Console::Error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
            response_string = "{\"error\": \"cURL request failed: " + std::string(curl_easy_strerror(res)) + "\"}";
        } else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            Console::Log("cURL request completed. HTTP Status Code: " + std::to_string(http_code));
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        Console::Error("Failed to initialize CURL for Gemini.");
        response_string = "{\"error\": \"CURL initialization failed\"}";
    }
    Console::Log("Gemini Raw Response: " + response_string);
    return response_string;
}

void AIPromptProcessor::PollAndProcessPendingCommands() {
    if (m_geminiFuture.valid()) {
        auto status = m_geminiFuture.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            std::string translatedCommand = m_geminiFuture.get();
            Console::Log("LLM (Async): Gemini task completed with response: " + translatedCommand);
            if (translatedCommand == "UNKNOWN_COMMAND" || translatedCommand.empty()) {
                Console::Log("The command is: " + translatedCommand);
                Console::Error("LLM (Async): Could not understand or translate the command.");
            } else {
                Console::Log("LLM (Async): Received translated command: " + translatedCommand);
                HandlePrompt(translatedCommand, true);
            }
        } else if (status == std::future_status::timeout) {
        } else if (status == std::future_status::deferred) {
            Console::Warn("LLM (Async): Gemini task was deferred.");
        }
    }
}

std::string AIPromptProcessor::TranslateNaturalLanguageToCommand(const std::string& natural_language_query) {
    Console::Log("Translating natural language: " + natural_language_query);
    std::string system_prompt = 
        "You are an AI assistant that translates natural language into structured game engine commands. "
        "The available commands are: \n"
        "1. create entity <name> at <x> <y> sprite <texture_id> [width <w>] [height <h>]\n"
        "2. script entity <name> with <script_path.lua>\n"
        "3. move entity <name> to <x> <y>\n"
        "4. delete entity <name>\n"
        "5. gemini_script <prompt for lua script> (Use this if the user asks to generate a script or code)\n"
        "6. gemini_modify <entity_name> <modification prompt> (Use this if the user asks to change an existing entity in a complex way not covered by other commands)\n"
        "Based on the user's input, provide ONLY the corresponding structured command. "
        "If the input doesn't match any command try to find the closer one, or if it is very very ambiguous, output 'UNKNOWN_COMMAND'. "
        "For 'create', if width/height are not specified, they can be omitted. Assume common game object names if not specified (e.g., 'player', 'enemy', 'block'). "
        "For 'script', the path should be a valid .lua path, if the user just gives a script name, assume it's in 'assets/Scripts/'."
        "User input: " + natural_language_query;

    std::string geminiResponse = ProcessGeminiPrompt(system_prompt);

    Console::Log("Gemini Response: " + geminiResponse);

    std::string translated_command = "UNKNOWN_COMMAND";
    try {
        auto json = nlohmann::json::parse(geminiResponse);
        if (json.contains("candidates") &&
            !json["candidates"].empty() &&
            json["candidates"][0].contains("content") &&
            json["candidates"][0]["content"].contains("parts") &&
            !json["candidates"][0]["content"]["parts"].empty() &&
            json["candidates"][0]["content"]["parts"][0].contains("text")) {
            translated_command = json["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
            // Remove trailing newline if present
            if (!translated_command.empty() && translated_command.back() == '\n') {
                translated_command.pop_back();
            }
        }
    } catch (const std::exception& e) {
        Console::Error(std::string("Failed to parse Gemini response JSON: ") + e.what());
    }
    Console::Log("Translated command: " + translated_command);
    return translated_command;
}


void AIPromptProcessor::GenerateScriptFromGemini(const std::string& scriptPrompt, std::string& outScriptPath) {
    Console::Log("Generating script with prompt: " + scriptPrompt);
    std::string geminiResponse = ProcessGeminiPrompt("Generate a Lua script for a game entity based on this description: " + scriptPrompt + ". The script should be self-contained and primarily define an update(deltaTime) function if applicable, and an init() function. Only output the Lua code itself, no explanations or markdown.");
    
    std::string script_content = "-- Failed to parse Gemini response or extract script.";

    size_t text_start = geminiResponse.find("\\\"text\\\": \\\""); 
    if (text_start != std::string::npos) {
        text_start += 8; 
        size_t text_end = geminiResponse.find("\\\"", text_start); 
        if (text_end != std::string::npos) {
            script_content = geminiResponse.substr(text_start, text_end - text_start);
            size_t pos = 0;
            while((pos = script_content.find("\\\\n", pos)) != std::string::npos) { 
                script_content.replace(pos, 2, "\\n");
                pos += 1;
            }
            pos = 0;
            while((pos = script_content.find("\\\\\\\"", pos)) != std::string::npos) { 
                script_content.replace(pos, 2, "\\\"");
                pos += 1;
            }
        }
    }

    std::string dirPath = "assets/Scripts/generated/";
    outScriptPath = dirPath + "generated_script_" + std::to_string(time(0)) + ".lua";
    std::ofstream file(outScriptPath);
    if (file.is_open()) {
        file << "-- Script generated by Gemini from prompt: " << scriptPrompt << "\\n";
        file << script_content;
        file.close();
        Console::Log("Script generated and saved to: " + outScriptPath);
    } else {
        Console::Error("Failed to open file to save generated script: " + outScriptPath);
        outScriptPath = ""; 
    }
}

void AIPromptProcessor::ModifyComponentFromGemini(Entity entity, const std::string& modificationPrompt) {
    Console::Log("Modifying entity " + std::to_string(entity) + " with prompt: " + modificationPrompt);
    Console::Warn("ModifyComponentFromGemini is not fully implemented.");
    std::string geminiResponse = ProcessGeminiPrompt("Given an entity, how would you modify its components based on the following request: " + modificationPrompt + ". Describe the changes needed.");
    Console::Log("Gemini suggestion for modification: " + geminiResponse);
}

void AIPromptProcessor::HandlePrompt(const std::string& prompt, bool isTranslatedCommand) {
    if (!isTranslatedCommand) {
        if (m_geminiFuture.valid() && m_geminiFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            Console::Warn("LLM: AI is already processing a command. Please wait.");
            return;
        }
        Console::Log("LLM: Submitting natural language prompt for asynchronous translation: " + prompt);

        m_geminiFuture = std::async(std::launch::async, [this, prompt]() {
            return TranslateNaturalLanguageToCommand(prompt);
        });
        return;
    }

    Console::Log("Executing translated/direct command: " + prompt);
    std::istringstream iss(prompt);
    std::string command;
    iss >> command;

    if (command == "gemini_script") {
        std::string script_prompt_parts;
        std::string part;
        while(iss >> part) {
            script_prompt_parts += part + " ";
        }
        if (!script_prompt_parts.empty()) {
            script_prompt_parts.pop_back(); 
            std::string generatedPath;
            GenerateScriptFromGemini(script_prompt_parts, generatedPath);
            if (!generatedPath.empty()) {
                 Console::Log("Gemini generated script at: " + generatedPath);
            }
        } else {
            Console::Warn("Gemini script prompt is empty. Usage: gemini_script <your detailed prompt for a lua script>");
        }
        return;
    } else if (command == "gemini_modify") {
        std::string entityName;
        iss >> entityName;
        Entity targetEntity = m_findEntityByNameFunc(entityName);
        if (targetEntity == NO_ENTITY_SELECTED) {
            Console::Error("LLM/Gemini: Entity '" + entityName + "' not found for modification.");
            return;
        }
        std::string modification_prompt_parts;
        std::string part;
        while(iss >> part) {
            modification_prompt_parts += part + " ";
        }
        if (!modification_prompt_parts.empty()) {
            modification_prompt_parts.pop_back(); 
            ModifyComponentFromGemini(targetEntity, modification_prompt_parts);
        } else {
            Console::Warn("Gemini modification prompt is empty. Usage: gemini_modify <entity_name> <your detailed modification request>");
        }
        return;
    }
    
    if (command == "create") {
        std::string type, entityName, at_keyword, sprite_keyword, textureId;
        float x = 0.0f, y = 0.0f; 
        float w = 32.0f, h = 32.0f; 

        iss >> type;
        if (type != "entity") {
            Console::Error("LLM: Expected 'entity' after 'create'. Usage: create entity <name> at <x> <y> sprite <texture_id>");
            return;
        }
        iss >> entityName >> at_keyword >> x >> y >> sprite_keyword >> textureId;

        if (at_keyword != "at" || sprite_keyword != "sprite") {
            Console::Error("LLM: Invalid 'create' syntax. Usage: create entity <name> at <x> <y> sprite <texture_id> [width <w>] [height <h>]");
            return;
        }

        std::string opt_param;
        while(iss >> opt_param) {
            if (opt_param == "width") iss >> w;
            else if (opt_param == "height") iss >> h;
        }

        if (m_findEntityByNameFunc(entityName) != NO_ENTITY_SELECTED) {
            Console::Warn("LLM: Entity with name '" + entityName + "' already exists.");
            return;
        }

        if (!m_assetManager->getTexture(textureId)) { 
            Console::Warn("LLM: Texture ID '" + textureId + "' not found. Entity might be invisible.");
        }

        Entity newEntity = m_entityManager->createEntity();
        m_componentManager->addComponent<NameComponent>(newEntity, NameComponent(entityName));
        TransformComponent transformComp(x, y, w, h, 0.0f, 0);
        m_componentManager->addComponent<TransformComponent>(newEntity, transformComp);
        m_componentManager->addComponent<SpriteComponent>(newEntity, SpriteComponent{textureId});

        if (m_componentManager->isComponentRegistered<RigidbodyComponent>()) {
            RigidbodyComponent rigidBodyComp; 
            m_componentManager->addComponent<RigidbodyComponent>(newEntity, rigidBodyComp);
        }

        if (m_componentManager->isComponentRegistered<ColliderComponent>()) {

            m_componentManager->addComponent<ColliderComponent>(newEntity, ColliderComponent(w, h)); 
            Console::Log("LLM: Added ColliderComponent to '" + entityName + "' with width: " + std::to_string(w) + ", height: " + std::to_string(h));
        } else {
            Console::Warn("LLM: ColliderComponent not registered. Entity '" + entityName + "' will not have collision.");
        }


        Signature sig;
        sig.set(m_componentManager->getComponentType<NameComponent>());
        sig.set(m_componentManager->getComponentType<TransformComponent>());
        sig.set(m_componentManager->getComponentType<SpriteComponent>());
        if (m_componentManager->isComponentRegistered<RigidbodyComponent>() && m_componentManager->hasComponent<RigidbodyComponent>(newEntity)) {
            sig.set(m_componentManager->getComponentType<RigidbodyComponent>());
        }
        if (m_componentManager->isComponentRegistered<ColliderComponent>() && m_componentManager->hasComponent<ColliderComponent>(newEntity)) {
            sig.set(m_componentManager->getComponentType<ColliderComponent>());
        }
        m_entityManager->setSignature(newEntity, sig);
        m_systemManager->entitySignatureChanged(newEntity, sig);
        Console::Log("LLM: Created entity '" + entityName + "' at (" + std::to_string(x) + "," + std::to_string(y) + ") with sprite '" + textureId + "'.");

    } else if (command == "script") {
        std::string type, entityName, with_keyword, scriptPath;
        iss >> type;
        if (type != "entity") {
             Console::Error("LLM: Expected 'entity' after 'script'. Usage: script entity <name> with <script_path.lua>");
            return;
        }
        iss >> entityName >> with_keyword >> scriptPath;

        if (with_keyword != "with") {
            Console::Error("LLM: Invalid 'script' syntax. Usage: script entity <name> with <script_path.lua>");
            return;
        }

        Entity targetEntity = m_findEntityByNameFunc(entityName);
        if (targetEntity == NO_ENTITY_SELECTED) {
            Console::Error("LLM: Entity '" + entityName + "' not found for scripting.");
            return;
        }

        if (!m_componentManager->isComponentRegistered<ScriptComponent>()) {
            Console::Error("LLM: ScriptComponent is not registered with the ComponentManager.");
            return;
        }

        if (!m_componentManager->hasComponent<ScriptComponent>(targetEntity)) {
            m_componentManager->addComponent<ScriptComponent>(targetEntity, ScriptComponent(scriptPath));
            Signature sig = m_entityManager->getSignature(targetEntity);
            sig.set(m_componentManager->getComponentType<ScriptComponent>());
            m_entityManager->setSignature(targetEntity, sig);
            m_systemManager->entitySignatureChanged(targetEntity, sig);
            Console::Log("LLM: Added script '" + scriptPath + "' to entity '" + entityName + "'.");
        } else {
            auto& scriptComp = m_componentManager->getComponent<ScriptComponent>(targetEntity);
            scriptComp.scriptPath = scriptPath;
            Console::Log("LLM: Updated script for entity '" + entityName + "' to '" + scriptPath + "'.");
        }

    } else if (command == "move") {
        std::string type, entityName, to_keyword;
        float x, y; 
        iss >> type;
        if (type != "entity") {
             Console::Error("LLM: Expected 'entity' after 'move'. Usage: move entity <name> to <x> <y>");
            return;
        }
        iss >> entityName >> to_keyword >> x >> y;

        if (to_keyword != "to") {
            Console::Error("LLM: Invalid 'move' syntax. Usage: move entity <name> to <x> <y>");
            return;
        }
        Entity targetEntity = m_findEntityByNameFunc(entityName);
        if (targetEntity == NO_ENTITY_SELECTED) {
            Console::Error("LLM: Entity '" + entityName + "' not found to move.");
            return;
        }
        if (m_componentManager->hasComponent<TransformComponent>(targetEntity)) {
            auto& transform = m_componentManager->getComponent<TransformComponent>(targetEntity);
            transform.x = x;
            transform.y = y;
            Console::Log("LLM: Moved entity '" + entityName + "' to (" + std::to_string(x) + "," + std::to_string(y) + ").");
        } else {
            Console::Error("LLM: Entity '" + entityName + "' does not have a TransformComponent to move.");
        }
    } else if (command == "delete") {
        std::string type, entityName;
        iss >> type;
         if (type != "entity") {
             Console::Error("LLM: Expected 'entity' after 'delete'. Usage: delete entity <name>");
            return;
        }
        iss >> entityName;
        Entity targetEntity = m_findEntityByNameFunc(entityName);
        if (targetEntity == NO_ENTITY_SELECTED) {
            Console::Error("LLM: Entity '" + entityName + "' not found to delete.");
            return;
        }
        m_entityManager->destroyEntity(targetEntity);
        Console::Log("LLM: Deleted entity '" + entityName + "'.");

    } else {
        Console::Error("LLM: Unknown command '" + command + "'. Supported: create, script, move, delete, gemini_script, gemini_modify");
    }

}

void AIPromptProcessor::renderAIPromptUI() {
    ImGui::Text("AI Prompt (Natural Language or Direct Command)"); 
    if (ImGui::InputText("##llmPrompt", m_llmPromptBuffer, sizeof(m_llmPromptBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (strlen(m_llmPromptBuffer) > 0) {
            Console::Log("User input: " + std::string(m_llmPromptBuffer));
            HandlePrompt(m_llmPromptBuffer, false); 
            m_llmPromptBuffer[0] = '\0'; 
        } else {
            Console::Warn("Prompt is empty.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Execute##llm")) {
        if (strlen(m_llmPromptBuffer) > 0) {
            Console::Log("User input: " + std::string(m_llmPromptBuffer));
            HandlePrompt(m_llmPromptBuffer, false); 
            m_llmPromptBuffer[0] = '\0'; 
        } else {
            Console::Warn("Prompt is empty.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear##llm")) {
        m_llmPromptBuffer[0] = '\0'; 
    }
    ImGui::TextWrapped("Local Commands: create entity <name> at <x> <y> sprite <id> | script entity <name> with <path.lua> | move <name> to <x> <y> | delete <name>");
    ImGui::TextWrapped("Gemini: gemini_script <prompt for lua script> | gemini_modify <entity_name> <modification prompt>");
}
