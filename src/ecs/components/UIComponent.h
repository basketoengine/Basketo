#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <SDL2/SDL.h>
#include "../Entity.h"
#include "../../../vendor/nlohmann/json.hpp"

// Forward declarations
class UISystem;

// UI element types
enum class UIElementType {
    PANEL,
    BUTTON,
    TEXT,
    SLIDER,
    INPUT_FIELD,
    IMAGE,
    CHECKBOX,
    DROPDOWN
};

// UI anchor types for positioning
enum class UIAnchor {
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
};

// Layout types for containers
enum class UILayoutType {
    NONE,
    HORIZONTAL,
    VERTICAL,
    GRID
};

// UI element state
enum class UIState {
    NORMAL,
    HOVERED,
    PRESSED,
    FOCUSED,
    DISABLED
};

// UI styling information
struct UIStyle {
    SDL_Color backgroundColor = {200, 200, 200, 255};
    SDL_Color borderColor = {100, 100, 100, 255};
    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Color hoverColor = {220, 220, 220, 255};
    SDL_Color pressedColor = {180, 180, 180, 255};
    SDL_Color disabledColor = {150, 150, 150, 128};
    
    int borderWidth = 1;
    int cornerRadius = 0;
    int fontSize = 16;
    std::string fontFamily = "Roboto-Regular_16_16";
    
    // Padding and margins
    int paddingLeft = 5;
    int paddingRight = 5;
    int paddingTop = 5;
    int paddingBottom = 5;
    int marginLeft = 0;
    int marginRight = 0;
    int marginTop = 0;
    int marginBottom = 0;
};

// UI event callback types
using UIEventCallback = std::function<void(Entity)>;
using UIValueCallback = std::function<void(Entity, float)>;
using UITextCallback = std::function<void(Entity, const std::string&)>;

// Base UI component
struct UIComponent {
    UIElementType type = UIElementType::PANEL;
    UIState state = UIState::NORMAL;
    UIAnchor anchor = UIAnchor::TOP_LEFT;
    
    // Position and size (relative to parent or screen)
    float x = 0.0f;
    float y = 0.0f;
    float width = 100.0f;
    float height = 30.0f;
    
    // Calculated absolute position (set by UI system)
    float absoluteX = 0.0f;
    float absoluteY = 0.0f;
    
    // Hierarchy
    Entity parent = NO_ENTITY;
    std::vector<Entity> children;
    
    // Visibility and interaction
    bool visible = true;
    bool interactive = true;
    bool focusable = false;
    int zOrder = 0;
    
    // Layout properties
    UILayoutType layoutType = UILayoutType::NONE;
    float layoutSpacing = 5.0f;
    int gridColumns = 1;
    
    // Styling
    UIStyle style;
    
    // Event callbacks
    UIEventCallback onClicked;
    UIEventCallback onHover;
    UIEventCallback onFocus;
    UIEventCallback onBlur;
    
    // Animation properties
    bool animating = false;
    float animationTime = 0.0f;
    float animationDuration = 0.3f;
    
    UIComponent() = default;
    UIComponent(UIElementType t) : type(t) {}
    
    // Helper methods
    bool containsPoint(float px, float py) const {
        return px >= absoluteX && px <= absoluteX + width &&
               py >= absoluteY && py <= absoluteY + height;
    }
    
    void addChild(Entity child) {
        children.push_back(child);
    }
    
    void removeChild(Entity child) {
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
    }
    
    SDL_Rect getRect() const {
        return {static_cast<int>(absoluteX), static_cast<int>(absoluteY), 
                static_cast<int>(width), static_cast<int>(height)};
    }
    
    SDL_Color getCurrentBackgroundColor() const {
        switch (state) {
            case UIState::HOVERED: return style.hoverColor;
            case UIState::PRESSED: return style.pressedColor;
            case UIState::DISABLED: return style.disabledColor;
            default: return style.backgroundColor;
        }
    }
};

// Button-specific component
struct UIButtonComponent {
    std::string text = "Button";
    bool pressed = false;
    bool wasPressed = false; // For detecting click events
    
    UIButtonComponent() = default;
    UIButtonComponent(const std::string& buttonText) : text(buttonText) {}
};

// Text-specific component
struct UITextComponent {
    std::string text = "Text";
    bool wordWrap = false;
    bool autoSize = true;
    
    UITextComponent() = default;
    UITextComponent(const std::string& textContent) : text(textContent) {}
};

// Slider-specific component
struct UISliderComponent {
    float value = 0.5f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    bool dragging = false;
    
    UIValueCallback onValueChanged;
    
    UISliderComponent() = default;
    UISliderComponent(float min, float max, float val = 0.5f) 
        : minValue(min), maxValue(max), value(val) {}
    
    float getNormalizedValue() const {
        return (value - minValue) / (maxValue - minValue);
    }
    
    void setNormalizedValue(float normalized) {
        value = minValue + normalized * (maxValue - minValue);
        value = std::max(minValue, std::min(maxValue, value));
    }
};

// Input field-specific component
struct UIInputFieldComponent {
    std::string text = "";
    std::string placeholder = "Enter text...";
    bool focused = false;
    int cursorPosition = 0;
    float cursorBlinkTime = 0.0f;
    bool showCursor = true;
    int maxLength = 256;
    
    UITextCallback onTextChanged;
    UIEventCallback onEnterPressed;
    
    UIInputFieldComponent() = default;
    UIInputFieldComponent(const std::string& placeholderText) : placeholder(placeholderText) {}
    
    void insertText(const std::string& insertText) {
        if (text.length() + insertText.length() <= maxLength) {
            text.insert(cursorPosition, insertText);
            cursorPosition += insertText.length();
        }
    }
    
    void deleteCharacter() {
        if (cursorPosition > 0) {
            text.erase(cursorPosition - 1, 1);
            cursorPosition--;
        }
    }
    
    void moveCursor(int delta) {
        cursorPosition = std::max(0, std::min(static_cast<int>(text.length()), cursorPosition + delta));
    }
};

// Panel-specific component (container)
struct UIPanelComponent {
    bool clipChildren = true;
    SDL_Color backgroundColor = {240, 240, 240, 255};
    
    UIPanelComponent() = default;
};

// Image-specific component
struct UIImageComponent {
    std::string textureId = "";
    bool preserveAspectRatio = true;
    
    UIImageComponent() = default;
    UIImageComponent(const std::string& texture) : textureId(texture) {}
};

// JSON serialization for UIComponent
inline void to_json(nlohmann::json& j, const UIComponent& comp) {
    j = nlohmann::json{
        {"type", static_cast<int>(comp.type)},
        {"anchor", static_cast<int>(comp.anchor)},
        {"x", comp.x},
        {"y", comp.y},
        {"width", comp.width},
        {"height", comp.height},
        {"visible", comp.visible},
        {"interactive", comp.interactive},
        {"focusable", comp.focusable},
        {"zOrder", comp.zOrder},
        {"layoutType", static_cast<int>(comp.layoutType)},
        {"layoutSpacing", comp.layoutSpacing},
        {"gridColumns", comp.gridColumns}
    };
    
    // Serialize style
    j["style"] = {
        {"backgroundColor", {comp.style.backgroundColor.r, comp.style.backgroundColor.g, comp.style.backgroundColor.b, comp.style.backgroundColor.a}},
        {"borderColor", {comp.style.borderColor.r, comp.style.borderColor.g, comp.style.borderColor.b, comp.style.borderColor.a}},
        {"textColor", {comp.style.textColor.r, comp.style.textColor.g, comp.style.textColor.b, comp.style.textColor.a}},
        {"borderWidth", comp.style.borderWidth},
        {"cornerRadius", comp.style.cornerRadius},
        {"fontSize", comp.style.fontSize},
        {"fontFamily", comp.style.fontFamily},
        {"paddingLeft", comp.style.paddingLeft},
        {"paddingRight", comp.style.paddingRight},
        {"paddingTop", comp.style.paddingTop},
        {"paddingBottom", comp.style.paddingBottom}
    };
}

inline void from_json(const nlohmann::json& j, UIComponent& comp) {
    comp.type = static_cast<UIElementType>(j.value("type", 0));
    comp.anchor = static_cast<UIAnchor>(j.value("anchor", 0));
    comp.x = j.value("x", 0.0f);
    comp.y = j.value("y", 0.0f);
    comp.width = j.value("width", 100.0f);
    comp.height = j.value("height", 30.0f);
    comp.visible = j.value("visible", true);
    comp.interactive = j.value("interactive", true);
    comp.focusable = j.value("focusable", false);
    comp.zOrder = j.value("zOrder", 0);
    comp.layoutType = static_cast<UILayoutType>(j.value("layoutType", 0));
    comp.layoutSpacing = j.value("layoutSpacing", 5.0f);
    comp.gridColumns = j.value("gridColumns", 1);
    
    // Deserialize style
    if (j.contains("style")) {
        auto styleJson = j["style"];
        if (styleJson.contains("backgroundColor")) {
            auto bg = styleJson["backgroundColor"];
            comp.style.backgroundColor = {bg[0], bg[1], bg[2], bg[3]};
        }
        if (styleJson.contains("borderColor")) {
            auto bc = styleJson["borderColor"];
            comp.style.borderColor = {bc[0], bc[1], bc[2], bc[3]};
        }
        if (styleJson.contains("textColor")) {
            auto tc = styleJson["textColor"];
            comp.style.textColor = {tc[0], tc[1], tc[2], tc[3]};
        }
        comp.style.borderWidth = styleJson.value("borderWidth", 1);
        comp.style.cornerRadius = styleJson.value("cornerRadius", 0);
        comp.style.fontSize = styleJson.value("fontSize", 16);
        comp.style.fontFamily = styleJson.value("fontFamily", "Roboto-Regular_16_16");
        comp.style.paddingLeft = styleJson.value("paddingLeft", 5);
        comp.style.paddingRight = styleJson.value("paddingRight", 5);
        comp.style.paddingTop = styleJson.value("paddingTop", 5);
        comp.style.paddingBottom = styleJson.value("paddingBottom", 5);
    }
}
