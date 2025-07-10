#pragma once

#include "../System.h"
#include "../ComponentManager.h"
#include "../components/UIComponent.h"
#include "../components/TransformComponent.h"
#include "../components/EventComponent.h"
#include "../EntityManager.h"
#include "../../AssetManager.h"
#include "../../InputManager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <queue>
#include <unordered_map>
#include <iostream>

class UISystem : public System {
public:
    UISystem();
    ~UISystem() = default;
    
    void update(ComponentManager* componentManager, float deltaTime);
    void render(SDL_Renderer* renderer, ComponentManager* componentManager);
    void handleInput(ComponentManager* componentManager, const SDL_Event& event);
    
    // UI element creation helpers (need EntityManager)
    Entity createButton(ComponentManager* componentManager, EntityManager* entityManager,
                       const std::string& text, float x, float y, float width, float height);
    Entity createText(ComponentManager* componentManager, EntityManager* entityManager,
                     const std::string& text, float x, float y);
    Entity createPanel(ComponentManager* componentManager, EntityManager* entityManager,
                      float x, float y, float width, float height);
    Entity createSlider(ComponentManager* componentManager, EntityManager* entityManager,
                       float min, float max, float value, float x, float y, float width, float height);
    Entity createInputField(ComponentManager* componentManager, EntityManager* entityManager,
                           const std::string& placeholder, float x, float y, float width, float height);
    Entity createImage(ComponentManager* componentManager, EntityManager* entityManager,
                      const std::string& textureId, float x, float y, float width, float height);
    
    // Layout management
    void updateLayout(Entity entity, ComponentManager* componentManager);
    void updateHierarchy(ComponentManager* componentManager);
    
    // Focus management
    void setFocus(Entity entity, ComponentManager* componentManager);
    Entity getFocusedEntity() const { return focusedEntity; }
    
    // Theme management
    void setTheme(const std::string& themeName);
    void applyTheme(Entity entity, ComponentManager* componentManager);
    
    // Performance metrics
    struct PerformanceMetrics {
        int totalUIElements = 0;
        int visibleUIElements = 0;
        int interactiveElements = 0;
        float updateTime = 0.0f;
        float renderTime = 0.0f;
        int layoutUpdates = 0;
        int eventHandles = 0;
    };
    
    const PerformanceMetrics& getMetrics() const { return metrics; }
    void resetMetrics();
    
    // Configuration
    void enableDebugMode(bool enable) { debugMode = enable; }
    void setScreenSize(int width, int height) { screenWidth = width; screenHeight = height; }
    
private:
    // Core UI processing
    void updateUIElement(Entity entity, ComponentManager* componentManager, float deltaTime);
    void updateAnimations(Entity entity, ComponentManager* componentManager, float deltaTime);
    void calculateAbsolutePositions(Entity entity, ComponentManager* componentManager);
    
    // Layout algorithms
    void applyHorizontalLayout(Entity parent, ComponentManager* componentManager);
    void applyVerticalLayout(Entity parent, ComponentManager* componentManager);
    void applyGridLayout(Entity parent, ComponentManager* componentManager);
    
    // Rendering methods
    void renderUIElement(Entity entity, ComponentManager* componentManager, SDL_Renderer* renderer);
    void renderPanel(Entity entity, const UIComponent& ui, ComponentManager* componentManager, SDL_Renderer* renderer);
    void renderButton(Entity entity, const UIComponent& ui, const UIButtonComponent& button, SDL_Renderer* renderer);
    void renderText(Entity entity, const UIComponent& ui, const UITextComponent& text, SDL_Renderer* renderer);
    void renderSlider(Entity entity, const UIComponent& ui, const UISliderComponent& slider, SDL_Renderer* renderer);
    void renderInputField(Entity entity, const UIComponent& ui, const UIInputFieldComponent& input, SDL_Renderer* renderer);
    void renderImage(Entity entity, const UIComponent& ui, const UIImageComponent& image, SDL_Renderer* renderer);
    
    // Input handling
    void handleMouseEvent(ComponentManager* componentManager, const SDL_Event& event);
    void handleKeyboardEvent(ComponentManager* componentManager, const SDL_Event& event);
    Entity findUIElementAt(float x, float y, ComponentManager* componentManager);
    void updateHoverStates(float mouseX, float mouseY, ComponentManager* componentManager);
    
    // Event generation
    void generateUIEvent(Entity entity, const std::string& eventName, ComponentManager* componentManager);
    void triggerCallback(Entity entity, UIEventCallback callback);
    
    // Utility methods
    void drawRectangle(SDL_Renderer* renderer, const SDL_Rect& rect, const SDL_Color& color, bool filled = true);
    void drawRoundedRectangle(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, const SDL_Color& color);
    void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, const SDL_Color& color, const std::string& fontId);
    SDL_Texture* getTextTexture(const std::string& text, const SDL_Color& color, const std::string& fontId);
    
    // System state
    AssetManager& assetManager;
    Entity focusedEntity = NO_ENTITY;
    Entity hoveredEntity = NO_ENTITY;
    Entity pressedEntity = NO_ENTITY;
    
    // Screen properties
    int screenWidth = 1024;
    int screenHeight = 768;
    
    // Configuration
    bool debugMode = false;
    std::string currentTheme = "default";
    
    // Performance tracking
    PerformanceMetrics metrics;
    std::chrono::high_resolution_clock::time_point frameStartTime;
    
    // Text rendering cache
    std::unordered_map<std::string, SDL_Texture*> textCache;
    
    // UI element sorting (by z-order)
    std::vector<Entity> sortedUIElements;
    
    // Helper methods
    void sortUIElementsByZOrder(ComponentManager* componentManager);
    void clearTextCache();
    std::string getTextCacheKey(const std::string& text, const SDL_Color& color, const std::string& fontId);
};
