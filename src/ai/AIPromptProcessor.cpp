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
#include <cstring>
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
    m_apiKeyBuffer[0] = '\0';
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

std::string AIPromptProcessor::ExtractGeminiResponseText(const std::string& jsonResponse) {
    try {
        auto json = nlohmann::json::parse(jsonResponse);
        if (json.contains("candidates") &&
            !json["candidates"].empty() &&
            json["candidates"][0].contains("content") &&
            json["candidates"][0]["content"].contains("parts") &&
            !json["candidates"][0]["content"]["parts"].empty() &&
            json["candidates"][0]["content"]["parts"][0].contains("text")) {
            return json["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
        } else {
            return "Invalid response format from Gemini API";
        }
    } catch (const std::exception& e) {
        return "JSON parsing error: " + std::string(e.what());
    }
}

void AIPromptProcessor::PollAndProcessPendingCommands() {
    if (m_geminiFuture.valid()) {
        auto status = m_geminiFuture.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            std::string translatedCommand = m_geminiFuture.get();
            Console::Log("LLM (Async): Gemini task completed with response: " + translatedCommand);
            m_isProcessing = false;

            if (translatedCommand == "UNKNOWN_COMMAND" || translatedCommand.empty()) {
                Console::Log("The command is: " + translatedCommand);
                Console::Error("LLM (Async): Could not understand or translate the command.");
                m_lastApiResponse = "Could not understand the command.\n\nPlease try rephrasing your request or be more specific about what you want to create or modify.";
            } else {
                Console::Log("LLM (Async): Received translated command: " + translatedCommand);

                // Format the response for better display
                std::string formattedResponse = "AI Generated Commands:\n\n";
                std::istringstream iss(translatedCommand);
                std::string line;
                int commandNum = 1;

                while (std::getline(iss, line)) {
                    if (!line.empty() && line.find_first_not_of(" \t\n\r") != std::string::npos) {
                        formattedResponse += std::to_string(commandNum++) + ". " + line + "\n";
                    }
                }

                m_lastApiResponse = formattedResponse;

                // In agent mode, automatically execute commands
                if (m_agentMode && !m_agentPaused) {
                    HandlePrompt(translatedCommand, true);
                    m_lastApiResponse = "Agent executed: " + formattedResponse;
                    m_lastAgentActivity = std::chrono::steady_clock::now();
                }
            }
        } else if (status == std::future_status::timeout) {
        } else if (status == std::future_status::deferred) {
            Console::Warn("LLM (Async): Gemini task was deferred.");
        }
    }

    // Update agent mode processing
    updateAgentMode();
}

std::string AIPromptProcessor::ListAssets() {
    std::string asset_list = "=== COMPREHENSIVE ASSET CONTEXT ===\n";

    // Textures with detailed info and loading status
    asset_list += "\nAVAILABLE TEXTURES:\n";

    // First, show loaded textures from AssetManager
    const auto& loadedTextures = m_assetManager->getAllTextures();
    if (!loadedTextures.empty()) {
        asset_list += "LOADED TEXTURES (ready to use immediately):\n";
        for (const auto& [textureId, texture] : loadedTextures) {
            asset_list += "  - '" + textureId + "' (LOADED & READY)\n";
        }
        asset_list += "\n";
    }

    // Then show available texture files
    asset_list += "TEXTURE FILES AVAILABLE:\n";
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator("assets/Textures")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string stem = entry.path().stem().string();
                std::string ext = entry.path().extension().string();

                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                    bool isLoaded = loadedTextures.find(filename) != loadedTextures.end();
                    asset_list += "  - '" + filename + "' " + (isLoaded ? "(LOADED)" : "(will auto-load)") + "\n";
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        asset_list += "  - No textures directory found\n";
    }

    asset_list += "\nTEXTURE USAGE:\n";
    asset_list += "  - Use FULL FILENAME including extension (e.g., 'mario.png', 'player.png')\n";
    asset_list += "  - Common textures: 'mario.png', 'player.png', 'background.jpg', 'marioblock.png'\n";
    asset_list += "  - NEVER use 'default' - always use actual filenames!\n";

    // Audio assets
    asset_list += "\nAUDIO FILES:\n";
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator("assets/Audio")) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string ext = entry.path().extension().string();
                if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") {
                    asset_list += "- " + entry.path().string() + "\n";
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        asset_list += "- No audio directory found\n";
    }

    // Scripts with content preview
    asset_list += "\nSCRIPTS (Lua files):\n";
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator("assets/Scripts")) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                asset_list += "- " + entry.path().string();

                // Add brief content preview
                std::ifstream file(entry.path());
                if (file.is_open()) {
                    std::string line;
                    int lineCount = 0;
                    asset_list += " (contains: ";
                    while (std::getline(file, line) && lineCount < 3) {
                        if (!line.empty() && line.find("--") != 0) {
                            asset_list += line.substr(0, 30) + "... ";
                            lineCount++;
                        }
                    }
                    asset_list += ")";
                }
                asset_list += "\n";
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        asset_list += "- No scripts directory found\n";
    }

    // Scenes
    asset_list += "\nSCENES:\n";
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator("assets/Scenes")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                asset_list += "- " + entry.path().string() + "\n";
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        asset_list += "- No scenes directory found\n";
    }

    return asset_list;
}

std::string AIPromptProcessor::BuildComprehensiveContext() {
    std::string context = "=== COMPLETE GAME ENGINE CONTEXT ===\n\n";

    // Asset context
    context += ListAssets();

    // Current scene state
    context += "\nCURRENT SCENE STATE:\n";
    if (m_entityManager) {
        const auto& entities = m_entityManager->getActiveEntities();
        context += "Active Entities (" + std::to_string(entities.size()) + "):\n";

        for (Entity entity : entities) {
            context += "- Entity " + std::to_string(entity) + ": ";

            // Check for name component
            if (m_componentManager && m_componentManager->hasComponent<NameComponent>(entity)) {
                auto& name = m_componentManager->getComponent<NameComponent>(entity);
                context += "'" + name.name + "' ";
            }

            // Check for transform
            if (m_componentManager && m_componentManager->hasComponent<TransformComponent>(entity)) {
                auto& transform = m_componentManager->getComponent<TransformComponent>(entity);
                context += "at (" + std::to_string(transform.x) + ", " + std::to_string(transform.y) + ") ";
                context += "size " + std::to_string(transform.width) + "x" + std::to_string(transform.height) + " ";
            }

            // Check for sprite
            if (m_componentManager && m_componentManager->hasComponent<SpriteComponent>(entity)) {
                auto& sprite = m_componentManager->getComponent<SpriteComponent>(entity);
                context += "sprite: " + sprite.textureId + " ";
            }

            // Check for script
            if (m_componentManager && m_componentManager->hasComponent<ScriptComponent>(entity)) {
                auto& script = m_componentManager->getComponent<ScriptComponent>(entity);
                context += "script: " + script.scriptPath + " ";
            }

            context += "\n";
        }
    }

    // Available commands with examples
    context += "\nAVAILABLE COMMANDS:\n";
    context += "1. create entity <name> at <x> <y> sprite <texture_id> [width <w>] [height <h>]\n";
    context += "2. move entity <name> to <x> <y>\n";
    context += "3. script entity <name> <script_path>\n";
    context += "4. gemini_script <script_name> <script_content>\n";
    context += "5. delete entity <name>\n";
    context += "6. set entity <name> size <width> <height>\n";
    context += "7. set entity <name> sprite <texture_id>\n";

    context += "\nEXAMPLES:\n";
    context += "- create entity player at 100 100 sprite mario width 64 height 64\n";
    context += "- gemini_script player_movement.lua 'function update() player.x = player.x + 1 end'\n";
    context += "- script entity player assets/Scripts/player_movement.lua\n";

    return context;
}

std::string AIPromptProcessor::TranslateNaturalLanguageToCommand(const std::string& natural_language_query) {
    Console::Log("Translating natural language: " + natural_language_query);
    std::string comprehensive_context = BuildComprehensiveContext();
    std::string system_prompt =
        "You are an ADVANCED AI GAME DEVELOPMENT ASSISTANT - like Cursor or Windsurf but for game engines.\n\n"

        "CORE CAPABILITIES:\n"
        "- You are FULLY AUTONOMOUS and can iterate on solutions\n"
        "- You understand the complete project context and available assets\n"
        "- You can create complex game features from scratch\n"
        "- You can debug, refine, and improve existing implementations\n"
        "- You think step-by-step and break down complex requests\n\n"

        "AVAILABLE COMMANDS:\n"
        "1. create entity <name> at <x> <y> sprite <texture_id> [width <w>] [height <h>]\n"
        "2. script entity <name> with <script_path.lua>\n"
        "3. move entity <name> to <x> <y>\n"
        "4. delete entity <name>\n"
        "5. gemini_script <prompt for lua script> (Use this to generate complete Lua scripts)\n"
        "6. gemini_modify <entity_name> <modification prompt> (Use this for complex entity modifications)\n\n"

        "INTELLIGENT BEHAVIOR:\n"
        "- ALWAYS use existing assets when appropriate (check the context below)\n"
        "- NEVER use 'default' as texture ID - use actual filenames like 'mario.png', 'player.png'\n"
        "- Use FULL FILENAMES including extensions for textures (e.g., 'mario.png' not 'mario')\n"
        "- Create comprehensive, working solutions, not just partial implementations\n"
        "- Write complete, functional Lua scripts with proper game logic\n"
        "- Position entities intelligently based on game context\n"
        "- Consider game design principles (spacing, balance, user experience)\n"
        "- If something doesn't work as expected, iterate and improve\n\n"

        "SCRIPT CREATION RULES:\n"
        "- ALL scripts must be in Lua (.lua extension)\n"
        "- ALWAYS use gemini_script to create script files BEFORE assigning them\n"
        "- Scripts should be saved in 'assets/Scripts/' directory\n"
        "- Write complete, functional scripts with proper game logic\n"
        "- Include comments explaining the script's purpose\n\n" +

        comprehensive_context + "\n\n" +
        "USER REQUEST: " + natural_language_query + "\n\n" +

        "THINK STEP BY STEP:\n"
        "1. Analyze what the user wants to achieve\n"
        "2. Check available assets that can be used\n"
        "3. Plan the complete implementation\n"
        "4. Generate the sequence of commands\n"
        "5. Ensure scripts are created before being assigned\n\n"

        "OUTPUT: Provide structured commands, one per line. Create complete, working game features!";

    std::string geminiResponse = ProcessGeminiPrompt(system_prompt);

    Console::Log("Gemini Raw Response: " + geminiResponse);

    std::string translated_command = "UNKNOWN_COMMAND";
    std::string display_response = "Failed to parse response";

    try {
        auto json = nlohmann::json::parse(geminiResponse);
        if (json.contains("candidates") &&
            !json["candidates"].empty() &&
            json["candidates"][0].contains("content") &&
            json["candidates"][0]["content"].contains("parts") &&
            !json["candidates"][0]["content"]["parts"].empty() &&
            json["candidates"][0]["content"]["parts"][0].contains("text")) {
            translated_command = json["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
            display_response = translated_command; // Store for UI display

            // Remove trailing newline if present
            if (!translated_command.empty() && translated_command.back() == '\n') {
                translated_command.pop_back();
            }
        } else {
            display_response = "Invalid response format from Gemini API";
        }
    } catch (const std::exception& e) {
        Console::Error(std::string("Failed to parse Gemini response JSON: ") + e.what());
        display_response = "JSON parsing error: " + std::string(e.what());
    }

    Console::Log("Translated command: " + translated_command);
    return translated_command;
}


void AIPromptProcessor::GenerateScriptFromGemini(const std::string& scriptPrompt, std::string& outScriptPath) {
    Console::Log("Generating script with prompt: " + scriptPrompt);

    // Extract the script name from the prompt, assuming the last word is the name.
    std::string scriptName;
    size_t last_space = scriptPrompt.find_last_of(" 	");
    if (last_space != std::string::npos) {
        scriptName = scriptPrompt.substr(last_space + 1);
    } else {
        scriptName = scriptPrompt; // Assume the whole prompt is the name if no spaces
    }
    // Ensure it ends with .lua
    if (scriptName.size() < 4 || scriptName.substr(scriptName.size() - 4) != ".lua") {
        scriptName += ".lua";
    }

    std::string geminiResponse = ProcessGeminiPrompt("Generate a Lua script for a game entity based on this description: " + scriptPrompt + ". The script should be self-contained and primarily define an update(deltaTime) function if applicable, and an init() function. Only output the Lua code itself, no explanations or markdown.");
    
    std::string script_content = "-- Failed to parse Gemini response or extract script.";

    try {
        auto json = nlohmann::json::parse(geminiResponse);
        if (json.contains("candidates") && !json["candidates"].empty() &&
            json["candidates"][0].contains("content") && json["candidates"][0]["content"].contains("parts") &&
            !json["candidates"][0]["content"]["parts"].empty() && json["candidates"][0]["content"]["parts"][0].contains("text")) {
            script_content = json["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
        }
    } catch (const nlohmann::json::parse_error& e) {
        Console::Error(std::string("Failed to parse Gemini script response JSON: ") + e.what());
        // Fallback to raw string search if JSON parsing fails
        size_t text_start = geminiResponse.find("\"text\": \"");
        if (text_start != std::string::npos) {
            text_start += 8; // length of \"text\": \"
            size_t text_end = geminiResponse.find("\"", text_start);
            if (text_end != std::string::npos) {
                script_content = geminiResponse.substr(text_start, text_end - text_start);
                // Unescape newlines and quotes
                size_t pos = 0;
                while ((pos = script_content.find("\\n", pos)) != std::string::npos) {
                    script_content.replace(pos, 2, "\n");
                    pos += 1;
                }
                pos = 0;
                while ((pos = script_content.find("\\\"", pos)) != std::string::npos) {
                    script_content.replace(pos, 2, "\"");
                    pos += 1;
                }
            }
        }
    }


    std::string dirPath = "assets/Scripts/";
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directories(dirPath);
    }
    outScriptPath = dirPath + scriptName;
    std::ofstream file(outScriptPath);
    if (file.is_open()) {
        file << "-- Script generated by Gemini from prompt: " << scriptPrompt << "\n";
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
        m_isProcessing = true;

        m_geminiFuture = std::async(std::launch::async, [this, prompt]() {
            return TranslateNaturalLanguageToCommand(prompt);
        });
        return;
    }

    Console::Log("Executing translated/direct command(s):\n" + prompt);
    std::istringstream iss(prompt);
    std::string line;
    std::string lastGeneratedScriptPath = "";

    while (std::getline(iss, line)) {
        // Trim leading/trailing whitespace from the line
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);

        if (line.empty()) {
            continue; // Skip empty lines
        }

        Console::Log("Executing command: " + line);
        std::istringstream line_iss(line);
        std::string command;
        line_iss >> command;

        if (command == "gemini_script") {
            std::string script_prompt_parts;
            std::string part;
            while(line_iss >> part) {
                script_prompt_parts += part + " ";
            }
            if (!script_prompt_parts.empty()) {
                script_prompt_parts.pop_back(); 
                GenerateScriptFromGemini(script_prompt_parts, lastGeneratedScriptPath);
                if (!lastGeneratedScriptPath.empty()) {
                     Console::Log("Gemini generated script at: " + lastGeneratedScriptPath);
                }
            } else {
                Console::Warn("Gemini script prompt is empty. Usage: gemini_script <your detailed prompt for a lua script>");
            }
            continue;
        } else if (command == "gemini_modify") {
            std::string entityName;
            line_iss >> entityName;
            Entity targetEntity = m_findEntityByNameFunc(entityName);
            if (targetEntity == NO_ENTITY_SELECTED) {
                Console::Error("LLM/Gemini: Entity '" + entityName + "' not found for modification.");
                continue; // Continue to next command
            }
            std::string modification_prompt_parts;
            std::string part;
            while(line_iss >> part) {
                modification_prompt_parts += part + " ";
            }
            if (!modification_prompt_parts.empty()) {
                modification_prompt_parts.pop_back(); 
                ModifyComponentFromGemini(targetEntity, modification_prompt_parts);
            } else {
                Console::Warn("Gemini modification prompt is empty. Usage: gemini_modify <entity_name> <your detailed modification request>");
            }
            continue; // Continue to next command
        }
        
        if (command == "create") {
            std::string type, entityName, at_keyword, sprite_keyword, textureId;
            float x = 0.0f, y = 0.0f; 
            float w = 32.0f, h = 32.0f; 

            line_iss >> type;
            if (type != "entity") {
                Console::Error("LLM: Expected 'entity' after 'create'. Usage: create entity <name> at <x> <y> sprite <texture_id>");
                continue;
            }
            line_iss >> entityName >> at_keyword >> x >> y >> sprite_keyword >> textureId;

            if (at_keyword != "at" || sprite_keyword != "sprite") {
                Console::Error("LLM: Invalid 'create' syntax. Usage: create entity <name> at <x> <y> sprite <texture_id> [width <w>] [height <h>]");
                continue;
            }

            std::string opt_param;
            while(line_iss >> opt_param) {
                if (opt_param == "width") line_iss >> w;
                else if (opt_param == "height") line_iss >> h;
            }

            if (m_findEntityByNameFunc(entityName) != NO_ENTITY_SELECTED) {
                Console::Warn("LLM: Entity with name '" + entityName + "' already exists.");
                continue;
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
            line_iss >> type;
            if (type != "entity") {
                 Console::Error("LLM: Expected 'entity' after 'script'. Usage: script entity <name> with <script_path.lua>");
                continue;
            }
            line_iss >> entityName >> with_keyword >> scriptPath;

            if (with_keyword != "with") {
                Console::Error("LLM: Invalid 'script' syntax. Usage: script entity <name> with <script_path.lua>");
                continue;
            }

            // If the script path is a placeholder and we have a recently generated script, use that.
            if (!lastGeneratedScriptPath.empty() && scriptPath.find(".lua") != std::string::npos) {
                 // Heuristic: if the script path given by the LLM contains the base name of the generated script, use the full generated path.
                std::filesystem::path genPath(lastGeneratedScriptPath);
                std::filesystem::path givenPath(scriptPath);
                if (scriptPath.find(genPath.stem().string()) != std::string::npos) {
                    Console::Log("LLM: Replacing script path '" + scriptPath + "' with last generated path '" + lastGeneratedScriptPath + "'.");
                    scriptPath = lastGeneratedScriptPath;
                }
            }


            Entity targetEntity = m_findEntityByNameFunc(entityName);
            if (targetEntity == NO_ENTITY_SELECTED) {
                Console::Error("LLM: Entity '" + entityName + "' not found for scripting.");
                continue;
            }

            if (!m_componentManager->isComponentRegistered<ScriptComponent>()) {
                Console::Error("LLM: ScriptComponent is not registered with the ComponentManager.");
                continue;
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
            lastGeneratedScriptPath = ""; // Reset after use

        } else if (command == "move") {
            std::string type, entityName, to_keyword;
            float x, y; 
            line_iss >> type;
            if (type != "entity") {
                 Console::Error("LLM: Expected 'entity' after 'move'. Usage: move entity <name> to <x> <y>");
                continue;
            }
            line_iss >> entityName >> to_keyword >> x >> y;

            if (to_keyword != "to") {
                Console::Error("LLM: Invalid 'move' syntax. Usage: move entity <name> to <x> <y>");
                continue;
            }
            Entity targetEntity = m_findEntityByNameFunc(entityName);
            if (targetEntity == NO_ENTITY_SELECTED) {
                Console::Error("LLM: Entity '" + entityName + "' not found to move.");
                continue;
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
            line_iss >> type;
             if (type != "entity") {
                 Console::Error("LLM: Expected 'entity' after 'delete'. Usage: delete entity <name>");
                continue;
            }
            line_iss >> entityName;
            Entity targetEntity = m_findEntityByNameFunc(entityName);
            if (targetEntity == NO_ENTITY_SELECTED) {
                Console::Error("LLM: Entity '" + entityName + "' not found to delete.");
                continue;
            }
            m_entityManager->destroyEntity(targetEntity);
            Console::Log("LLM: Deleted entity '" + entityName + "'.");

        } else {
            Console::Error("LLM: Unknown command '" + command + "'. Supported: create, script, move, delete, gemini_script, gemini_modify");
        }
    }
}

void AIPromptProcessor::renderAIPromptUI() {
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "ADVANCED AI ASSISTANT");
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Powered by Gemini 2.0 - Context-Aware & Iterative");
    ImGui::Separator();

    // Show capabilities
    if (ImGui::CollapsingHeader("AI Capabilities")) {
        ImGui::BulletText("Full project context awareness");
        ImGui::BulletText("Intelligent asset usage");
        ImGui::BulletText("Complete feature implementation");
        ImGui::BulletText("Iterative improvement & debugging");
        ImGui::BulletText("Autonomous agent mode");
        ImGui::BulletText("Game design best practices");
    }

    // Agent mode controls
    if (ImGui::CollapsingHeader("Autonomous Agent Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Like Cursor/Windsurf for Game Development");

        ImGui::Checkbox("Enable Agent Mode", &m_agentMode);
        ImGui::SameLine();
        if (ImGui::Button(m_agentPaused ? "Resume" : "Pause")) {
            pauseAgentMode(!m_agentPaused);
        }

        if (m_agentMode) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Agent Status: %s",
                m_agentPaused ? "PAUSED" : "ACTIVE & ITERATING");
            ImGui::Text("Tasks in queue: %zu", m_agentTaskQueue.size());
            ImGui::Text("Conversation history: %zu actions", m_conversationHistory.size());

            ImGui::Separator();
            if (ImGui::Button("Clear Queue")) {
                clearAgentTasks();
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Improvement Task")) {
                addAgentTask("Analyze the current scene and suggest improvements to make it more engaging");
            }

            // Show recent agent actions
            if (!m_conversationHistory.empty()) {
                ImGui::Text("Recent Actions:");
                ImGui::BeginChild("AgentHistory", ImVec2(0, 80), true);
                for (int i = std::max(0, (int)m_conversationHistory.size() - 5); i < (int)m_conversationHistory.size(); ++i) {
                    ImGui::BulletText("%s", m_conversationHistory[i].c_str());
                }
                ImGui::EndChild();
            }
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Enable agent mode for autonomous development");
        }
    }

    // Display AI responses or thinking status
    if (m_isProcessing) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "AI is thinking...");

        // Add a simple progress indicator
        static float progress = 0.0f;
        progress += 0.02f;
        if (progress > 1.0f) progress = 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "");
    } else if (!m_lastApiResponse.empty()) {
        ImGui::Text("AI Response:");
        ImGui::Separator();

        // Create a scrollable text area for the response
        ImGui::BeginChild("AIResponseArea", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x);

        // Color code different types of responses
        ImVec4 responseColor = ImVec4(0.6f, 1.0f, 0.6f, 1.0f); // Default green
        if (m_lastApiResponse.find("Could not understand") != std::string::npos) {
            responseColor = ImVec4(1.0f, 0.6f, 0.6f, 1.0f); // Red for errors
        } else if (m_lastApiResponse.find("Agent executed") != std::string::npos) {
            responseColor = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); // Blue for agent actions
        }

        ImGui::TextColored(responseColor, "%s", m_lastApiResponse.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndChild();

        // Action buttons
        if (!m_agentMode) { // Only show manual execution in non-agent mode
            if (ImGui::Button("Execute Commands")) {
                // Extract the actual commands from the formatted response
                std::string commandsToExecute = m_lastApiResponse;
                if (commandsToExecute.find("AI Generated Commands:") != std::string::npos) {
                    // Extract just the command part
                    size_t start = commandsToExecute.find("\n\n");
                    if (start != std::string::npos) {
                        commandsToExecute = commandsToExecute.substr(start + 2);
                        // Remove the numbering
                        std::string cleanCommands;
                        std::istringstream iss(commandsToExecute);
                        std::string line;
                        while (std::getline(iss, line)) {
                            size_t dotPos = line.find(". ");
                            if (dotPos != std::string::npos) {
                                cleanCommands += line.substr(dotPos + 2) + "\n";
                            }
                        }
                        commandsToExecute = cleanCommands;
                    }
                }
                HandlePrompt(commandsToExecute, true);
                m_lastApiResponse = ""; // Clear after execution
            }
            ImGui::SameLine();
        }
        if (ImGui::Button("Clear Response")) {
            m_lastApiResponse = "";
        }
        if (!m_agentMode) {
            ImGui::SameLine();
            if (ImGui::Button("Copy to Clipboard")) {
                ImGui::SetClipboardText(m_lastApiResponse.c_str());
            }
        }
    }

    ImGui::Separator();

    // Example prompts section
    if (ImGui::CollapsingHeader("Example Prompts")) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Try these advanced requests:");

        if (ImGui::Button("Create a platformer game")) {
            strcpy(m_llmPromptBuffer, "Create a complete platformer game using mario.png as the player, marioblock.png for platforms, and mario obstacle.png for enemies. Add jumping mechanics and collectibles!");
        }
        ImGui::SameLine();
        if (ImGui::Button("Build a space shooter")) {
            strcpy(m_llmPromptBuffer, "Create a space shooter game using player.png as the ship, create enemies and projectiles with movement and shooting mechanics");
        }

        if (ImGui::Button("Make an RPG character")) {
            strcpy(m_llmPromptBuffer, "Create an RPG character using mario.png with stats, inventory system, and level progression. Add NPCs using player.png");
        }
        ImGui::SameLine();
        if (ImGui::Button("Design a puzzle game")) {
            strcpy(m_llmPromptBuffer, "Design a puzzle game using marioblock.png for movable blocks, create switches and doors with mario.png textures");
        }

        if (ImGui::Button("Improve current scene")) {
            strcpy(m_llmPromptBuffer, "Analyze the current scene and add improvements to make it more engaging and interactive");
        }
        ImGui::SameLine();
        if (ImGui::Button("Add game mechanics")) {
            strcpy(m_llmPromptBuffer, "Add interesting game mechanics like physics, particle effects, or AI behaviors to existing entities");
        }
    }

    // Input prompt
    ImGui::Text("Your Request:");
    if (ImGui::InputText("##llmPrompt", m_llmPromptBuffer, sizeof(m_llmPromptBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (strlen(m_llmPromptBuffer) > 0) {
            Console::Log("User input: " + std::string(m_llmPromptBuffer));
            m_lastApiResponse = ""; // Clear previous response

            if (m_agentMode) {
                addAgentTask(std::string(m_llmPromptBuffer));
            } else {
                HandlePrompt(m_llmPromptBuffer, false);
            }
            m_llmPromptBuffer[0] = '\0';
        } else {
            Console::Warn("Prompt is empty.");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(m_agentMode ? "Add to Queue" : "Send")) {
        if (strlen(m_llmPromptBuffer) > 0) {
            Console::Log("User input: " + std::string(m_llmPromptBuffer));
            m_lastApiResponse = ""; // Clear previous response

            if (m_agentMode) {
                addAgentTask(std::string(m_llmPromptBuffer));
            } else {
                HandlePrompt(m_llmPromptBuffer, false);
            }
            m_llmPromptBuffer[0] = '\0';
        } else {
            Console::Warn("Prompt is empty.");
        }
    }

    // Gemini API Key Configuration Section
    ImGui::Separator();
    if (ImGui::CollapsingHeader("AI Configuration")) {
        ImGui::Text("Gemini AI Configuration");

        // API Key status indicator and direct input
        if (isApiKeyConfigured()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Gemini API Key Configured");
            ImGui::SameLine();
            if (ImGui::Button("Change Key##gemini")) {
                m_showApiKeyInput = !m_showApiKeyInput;
                if (m_showApiKeyInput) {
                    // Pre-fill with current API key (masked)
                    std::string maskedKey = std::string(m_apiKey.length(), '*');
                    strncpy(m_apiKeyBuffer, maskedKey.c_str(), sizeof(m_apiKeyBuffer) - 1);
                    m_apiKeyBuffer[sizeof(m_apiKeyBuffer) - 1] = '\0';
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ Gemini API Key Required");

            // Always show input field when not configured
            ImGui::Text("Enter Gemini API Key:");
            ImGui::SetNextItemWidth(-100); // Leave space for buttons
            if (ImGui::InputText("##gemini_api_key_main", m_apiKeyBuffer, sizeof(m_apiKeyBuffer), ImGuiInputTextFlags_Password)) {
                // Real-time validation could go here
            }

            ImGui::SameLine();
            if (ImGui::Button("Save##apikey_main")) {
                if (strlen(m_apiKeyBuffer) > 0) {
                    std::string newApiKey = std::string(m_apiKeyBuffer);
                    setApiKey(newApiKey);
                    saveApiKeyToConfig(newApiKey);
                    Console::Log("Gemini API Key saved successfully!");
                    m_apiKeyBuffer[0] = '\0';
                } else {
                    Console::Warn("Gemini API Key cannot be empty.");
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Help##apikey_help")) {
                m_showApiKeyInput = !m_showApiKeyInput;
            }
        }

        if (m_showApiKeyInput) {
            ImGui::Indent();
            ImGui::Spacing();

            // Instructions
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "How to get your Gemini API Key:");
            ImGui::BulletText("1. Visit: https://aistudio.google.com/app/apikey");
            ImGui::BulletText("2. Sign in with your Google account");
            ImGui::BulletText("3. Click 'Create API Key' and copy it");
            ImGui::BulletText("4. Paste it in the field below");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // API Key input field
            ImGui::Text("Gemini API Key:");
            ImGui::SetNextItemWidth(-100); // Leave space for buttons
            if (ImGui::InputText("##gemini_api_key", m_apiKeyBuffer, sizeof(m_apiKeyBuffer), ImGuiInputTextFlags_Password)) {
                // Real-time validation could go here
            }

            ImGui::SameLine();
            if (ImGui::Button("Save##apikey")) {
                if (strlen(m_apiKeyBuffer) > 0) {
                    std::string newApiKey = std::string(m_apiKeyBuffer);
                    setApiKey(newApiKey);
                    saveApiKeyToConfig(newApiKey);
                    Console::Log("Gemini API Key saved successfully!");
                    m_showApiKeyInput = false;
                    m_apiKeyBuffer[0] = '\0';
                } else {
                    Console::Warn("Gemini API Key cannot be empty.");
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Clear##apikey")) {
                m_apiKeyBuffer[0] = '\0';
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Your API key is stored securely in config.json");

            if (isApiKeyConfigured()) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Danger Zone");
                if (ImGui::Button("Remove Gemini API Key")) {
                    setApiKey("");
                    saveApiKeyToConfig("");
                    Console::Log("Gemini API Key removed.");
                    m_showApiKeyInput = false;
                    m_apiKeyBuffer[0] = '\0';
                }
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(This will disable AI features)");
            }

            ImGui::Unindent();
        }
    }
}

// API Key management methods
void AIPromptProcessor::setApiKey(const std::string& apiKey) {
    m_apiKey = apiKey;
}

bool AIPromptProcessor::isApiKeyConfigured() const {
    return !m_apiKey.empty();
}

void AIPromptProcessor::saveApiKeyToConfig(const std::string& apiKey) {
    std::string configPath = "config.json";
    nlohmann::json configJson;

    // Load existing config if it exists
    if (std::filesystem::exists(configPath)) {
        try {
            std::ifstream configFile(configPath);
            configFile >> configJson;
        } catch (const std::exception& e) {
            Console::Warn("Failed to read existing config.json, creating new one: " + std::string(e.what()));
            configJson = nlohmann::json::object();
        }
    } else {
        configJson = nlohmann::json::object();
    }

    // Update or remove the API key
    if (apiKey.empty()) {
        configJson.erase("gemini_api_key");
    } else {
        configJson["gemini_api_key"] = apiKey;
    }

    // Save the config
    try {
        std::ofstream configFile(configPath);
        configFile << configJson.dump(4) << std::endl;
        Console::Log("Configuration saved to " + configPath);
    } catch (const std::exception& e) {
        Console::Error("Failed to save config.json: " + std::string(e.what()));
    }
}

// Agent mode functionality
void AIPromptProcessor::enableAgentMode(bool enable) {
    m_agentMode = enable;
    if (enable) {
        Console::Log("Agent mode enabled - AI will operate autonomously");
        m_lastAgentActivity = std::chrono::steady_clock::now();
    } else {
        Console::Log("Agent mode disabled");
        clearAgentTasks();
    }
}

bool AIPromptProcessor::isAgentModeEnabled() const {
    return m_agentMode;
}

void AIPromptProcessor::pauseAgentMode(bool pause) {
    m_agentPaused = pause;
    if (pause) {
        Console::Log("Agent mode paused");
    } else {
        Console::Log("Agent mode resumed");
        m_lastAgentActivity = std::chrono::steady_clock::now();
    }
}

bool AIPromptProcessor::isAgentModePaused() const {
    return m_agentPaused;
}

void AIPromptProcessor::addAgentTask(const std::string& task) {
    if (m_agentTaskQueue.size() >= m_maxAgentTasks) {
        Console::Warn("Agent task queue is full, removing oldest task");
        m_agentTaskQueue.pop_front();
    }
    m_agentTaskQueue.push_back(task);
    Console::Log("Added task to agent queue: " + task);
}

void AIPromptProcessor::clearAgentTasks() {
    m_agentTaskQueue.clear();
    Console::Log("Agent task queue cleared");
}

void AIPromptProcessor::processNextAgentTask() {
    if (m_agentTaskQueue.empty() || m_agentPaused || !m_agentMode) {
        return;
    }

    if (m_geminiFuture.valid() && m_geminiFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return; // Still processing previous task
    }

    std::string nextTask = m_agentTaskQueue.front();
    m_agentTaskQueue.pop_front();

    Console::Log("Agent processing task: " + nextTask);

    // Build comprehensive context for the agent
    std::string agentPrompt = "AUTONOMOUS AGENT MODE - ADVANCED ITERATION\n\n";

    // Add comprehensive context
    agentPrompt += BuildComprehensiveContext() + "\n\n";

    // Add conversation history for context
    if (!m_conversationHistory.empty()) {
        agentPrompt += "PREVIOUS ACTIONS (for context):\n";
        for (size_t i = 0; i < m_conversationHistory.size(); ++i) {
            agentPrompt += std::to_string(i + 1) + ". " + m_conversationHistory[i] + "\n";
        }
        agentPrompt += "\n";
    }

    // Add the current task with enhanced instructions
    agentPrompt += "CURRENT TASK: " + nextTask + "\n\n";

    agentPrompt += "AGENT INSTRUCTIONS:\n";
    agentPrompt += "- You are in AUTONOMOUS AGENT MODE - be creative and comprehensive\n";
    agentPrompt += "- Analyze the current scene state and build upon existing entities\n";
    agentPrompt += "- Use available assets intelligently and create engaging gameplay\n";
    agentPrompt += "- If the task seems incomplete or could be improved, enhance it\n";
    agentPrompt += "- Create complete, working game features, not just basic implementations\n";
    agentPrompt += "- Think about user experience and game design principles\n";
    agentPrompt += "- If you notice issues with previous implementations, fix them\n\n";

    agentPrompt += "REMEMBER:\n";
    agentPrompt += "- Create scripts BEFORE assigning them to entities\n";
    agentPrompt += "- Position entities thoughtfully in the game world\n";
    agentPrompt += "- Make the game fun and interactive\n";
    agentPrompt += "- Use existing assets when appropriate\n\n";

    agentPrompt += "EXECUTE THE TASK NOW:";

    HandlePrompt(agentPrompt, false);

    // Add to conversation history with more detail
    std::string historyEntry = nextTask;
    if (m_lastApiResponse.find("right") != std::string::npos) {
        historyEntry += " (SUCCESS)";
    } else if (m_lastApiResponse.find("X") != std::string::npos) {
        historyEntry += " (ERROR - may need retry)";
    }

    m_conversationHistory.push_back(historyEntry);
    if (m_conversationHistory.size() > 15) { // Keep more history for better context
        m_conversationHistory.erase(m_conversationHistory.begin());
    }

    // Auto-add follow-up tasks for complex requests
    if (nextTask.find("create") != std::string::npos && nextTask.find("game") != std::string::npos) {
        // If creating a game, add follow-up tasks
        addAgentTask("Add interactive elements and improve gameplay");
        addAgentTask("Test and refine the game mechanics");
    }
}

void AIPromptProcessor::updateAgentMode() {
    if (!m_agentMode || m_agentPaused) {
        return;
    }

    // Check if we should process the next task
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastActivity = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastAgentActivity);

    // Process next task if we're not currently processing and enough time has passed
    if (timeSinceLastActivity.count() >= 2 && !m_isProcessing) { // 2 second delay between tasks
        processNextAgentTask();
    }
}
