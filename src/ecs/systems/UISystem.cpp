#include "UISystem.h"
#include <algorithm>
#include <chrono>
#include <cmath>

UISystem::UISystem() : assetManager(AssetManager::getInstance()) {
    std::cout << "[UISystem] Initialized" << std::endl;
}

void UISystem::update(ComponentManager* componentManager, float deltaTime) {
    frameStartTime = std::chrono::high_resolution_clock::now();
    
    // Reset frame metrics
    metrics.totalUIElements = 0;
    metrics.visibleUIElements = 0;
    metrics.interactiveElements = 0;
    metrics.layoutUpdates = 0;
    metrics.eventHandles = 0;
    
    // Sort UI elements by z-order for proper rendering
    sortUIElementsByZOrder(componentManager);
    
    // Update hierarchy and calculate absolute positions
    updateHierarchy(componentManager);
    
    // Update all UI elements
    for (auto const& entity : entities) {
        if (!componentManager->hasComponent<UIComponent>(entity)) continue;
        
        metrics.totalUIElements++;
        updateUIElement(entity, componentManager, deltaTime);
        
        auto& ui = componentManager->getComponent<UIComponent>(entity);
        if (ui.visible) {
            metrics.visibleUIElements++;
        }
        if (ui.interactive) {
            metrics.interactiveElements++;
        }
    }
    
    // Update performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    metrics.updateTime = std::chrono::duration<float, std::milli>(endTime - frameStartTime).count();
}

void UISystem::updateUIElement(Entity entity, ComponentManager* componentManager, float deltaTime) {
    auto& ui = componentManager->getComponent<UIComponent>(entity);
    
    if (!ui.visible) return;
    
    // Update animations
    updateAnimations(entity, componentManager, deltaTime);
    
    // Update specific UI element types
    switch (ui.type) {
        case UIElementType::SLIDER:
            if (componentManager->hasComponent<UISliderComponent>(entity)) {
                auto& slider = componentManager->getComponent<UISliderComponent>(entity);
                // Slider-specific updates would go here
            }
            break;
            
        case UIElementType::INPUT_FIELD:
            if (componentManager->hasComponent<UIInputFieldComponent>(entity)) {
                auto& input = componentManager->getComponent<UIInputFieldComponent>(entity);
                // Update cursor blink
                input.cursorBlinkTime += deltaTime;
                if (input.cursorBlinkTime >= 1.0f) {
                    input.showCursor = !input.showCursor;
                    input.cursorBlinkTime = 0.0f;
                }
            }
            break;
            
        default:
            break;
    }
    
    // Update layout if this is a container
    if (ui.layoutType != UILayoutType::NONE && !ui.children.empty()) {
        updateLayout(entity, componentManager);
        metrics.layoutUpdates++;
    }
}

void UISystem::updateAnimations(Entity entity, ComponentManager* componentManager, float deltaTime) {
    auto& ui = componentManager->getComponent<UIComponent>(entity);
    
    if (ui.animating) {
        ui.animationTime += deltaTime;
        if (ui.animationTime >= ui.animationDuration) {
            ui.animating = false;
            ui.animationTime = 0.0f;
        }
    }
}

void UISystem::updateHierarchy(ComponentManager* componentManager) {
    // Calculate absolute positions for all UI elements
    for (auto const& entity : entities) {
        if (!componentManager->hasComponent<UIComponent>(entity)) continue;
        
        auto& ui = componentManager->getComponent<UIComponent>(entity);
        if (ui.parent == NO_ENTITY) {
            // Root element - calculate from screen position
            calculateAbsolutePositions(entity, componentManager);
        }
    }
}

void UISystem::calculateAbsolutePositions(Entity entity, ComponentManager* componentManager) {
    auto& ui = componentManager->getComponent<UIComponent>(entity);
    
    if (ui.parent == NO_ENTITY) {
        // Root element - position relative to screen
        switch (ui.anchor) {
            case UIAnchor::TOP_LEFT:
                ui.absoluteX = ui.x;
                ui.absoluteY = ui.y;
                break;
            case UIAnchor::TOP_CENTER:
                ui.absoluteX = (screenWidth / 2.0f) + ui.x - (ui.width / 2.0f);
                ui.absoluteY = ui.y;
                break;
            case UIAnchor::TOP_RIGHT:
                ui.absoluteX = screenWidth - ui.width - ui.x;
                ui.absoluteY = ui.y;
                break;
            case UIAnchor::CENTER:
                ui.absoluteX = (screenWidth / 2.0f) + ui.x - (ui.width / 2.0f);
                ui.absoluteY = (screenHeight / 2.0f) + ui.y - (ui.height / 2.0f);
                break;
            case UIAnchor::BOTTOM_LEFT:
                ui.absoluteX = ui.x;
                ui.absoluteY = screenHeight - ui.height - ui.y;
                break;
            case UIAnchor::BOTTOM_CENTER:
                ui.absoluteX = (screenWidth / 2.0f) + ui.x - (ui.width / 2.0f);
                ui.absoluteY = screenHeight - ui.height - ui.y;
                break;
            case UIAnchor::BOTTOM_RIGHT:
                ui.absoluteX = screenWidth - ui.width - ui.x;
                ui.absoluteY = screenHeight - ui.height - ui.y;
                break;
            default:
                ui.absoluteX = ui.x;
                ui.absoluteY = ui.y;
                break;
        }
    } else {
        // Child element - position relative to parent
        if (componentManager->hasComponent<UIComponent>(ui.parent)) {
            auto& parentUI = componentManager->getComponent<UIComponent>(ui.parent);
            ui.absoluteX = parentUI.absoluteX + ui.x + parentUI.style.paddingLeft;
            ui.absoluteY = parentUI.absoluteY + ui.y + parentUI.style.paddingTop;
        }
    }
    
    // Recursively calculate positions for children
    for (Entity child : ui.children) {
        if (componentManager->hasComponent<UIComponent>(child)) {
            calculateAbsolutePositions(child, componentManager);
        }
    }
}

void UISystem::updateLayout(Entity entity, ComponentManager* componentManager) {
    auto& ui = componentManager->getComponent<UIComponent>(entity);
    
    switch (ui.layoutType) {
        case UILayoutType::HORIZONTAL:
            applyHorizontalLayout(entity, componentManager);
            break;
        case UILayoutType::VERTICAL:
            applyVerticalLayout(entity, componentManager);
            break;
        case UILayoutType::GRID:
            applyGridLayout(entity, componentManager);
            break;
        default:
            break;
    }
}

void UISystem::applyHorizontalLayout(Entity parent, ComponentManager* componentManager) {
    auto& parentUI = componentManager->getComponent<UIComponent>(parent);
    
    float currentX = parentUI.style.paddingLeft;
    float maxHeight = 0.0f;
    
    for (Entity child : parentUI.children) {
        if (!componentManager->hasComponent<UIComponent>(child)) continue;
        
        auto& childUI = componentManager->getComponent<UIComponent>(child);
        if (!childUI.visible) continue;
        
        childUI.x = currentX;
        childUI.y = parentUI.style.paddingTop;
        
        currentX += childUI.width + parentUI.layoutSpacing;
        maxHeight = std::max(maxHeight, childUI.height);
    }
    
    // Auto-resize parent if needed
    if (parentUI.width < currentX + parentUI.style.paddingRight) {
        parentUI.width = currentX + parentUI.style.paddingRight;
    }
    if (parentUI.height < maxHeight + parentUI.style.paddingTop + parentUI.style.paddingBottom) {
        parentUI.height = maxHeight + parentUI.style.paddingTop + parentUI.style.paddingBottom;
    }
}

void UISystem::applyVerticalLayout(Entity parent, ComponentManager* componentManager) {
    auto& parentUI = componentManager->getComponent<UIComponent>(parent);
    
    float currentY = parentUI.style.paddingTop;
    float maxWidth = 0.0f;
    
    for (Entity child : parentUI.children) {
        if (!componentManager->hasComponent<UIComponent>(child)) continue;
        
        auto& childUI = componentManager->getComponent<UIComponent>(child);
        if (!childUI.visible) continue;
        
        childUI.x = parentUI.style.paddingLeft;
        childUI.y = currentY;
        
        currentY += childUI.height + parentUI.layoutSpacing;
        maxWidth = std::max(maxWidth, childUI.width);
    }
    
    // Auto-resize parent if needed
    if (parentUI.height < currentY + parentUI.style.paddingBottom) {
        parentUI.height = currentY + parentUI.style.paddingBottom;
    }
    if (parentUI.width < maxWidth + parentUI.style.paddingLeft + parentUI.style.paddingRight) {
        parentUI.width = maxWidth + parentUI.style.paddingLeft + parentUI.style.paddingRight;
    }
}

void UISystem::applyGridLayout(Entity parent, ComponentManager* componentManager) {
    auto& parentUI = componentManager->getComponent<UIComponent>(parent);
    
    int columns = parentUI.gridColumns;
    int currentColumn = 0;
    int currentRow = 0;
    
    float cellWidth = (parentUI.width - parentUI.style.paddingLeft - parentUI.style.paddingRight - 
                      (columns - 1) * parentUI.layoutSpacing) / columns;
    
    for (Entity child : parentUI.children) {
        if (!componentManager->hasComponent<UIComponent>(child)) continue;
        
        auto& childUI = componentManager->getComponent<UIComponent>(child);
        if (!childUI.visible) continue;
        
        float x = parentUI.style.paddingLeft + currentColumn * (cellWidth + parentUI.layoutSpacing);
        float y = parentUI.style.paddingTop + currentRow * (childUI.height + parentUI.layoutSpacing);
        
        childUI.x = x;
        childUI.y = y;
        childUI.width = cellWidth;
        
        currentColumn++;
        if (currentColumn >= columns) {
            currentColumn = 0;
            currentRow++;
        }
    }
}

void UISystem::sortUIElementsByZOrder(ComponentManager* componentManager) {
    sortedUIElements.clear();
    
    for (auto const& entity : entities) {
        if (componentManager->hasComponent<UIComponent>(entity)) {
            sortedUIElements.push_back(entity);
        }
    }
    
    // Sort by z-order (higher z-order renders on top)
    std::sort(sortedUIElements.begin(), sortedUIElements.end(),
        [componentManager](Entity a, Entity b) {
            auto& uiA = componentManager->getComponent<UIComponent>(a);
            auto& uiB = componentManager->getComponent<UIComponent>(b);
            return uiA.zOrder < uiB.zOrder;
        });
}

void UISystem::render(SDL_Renderer* renderer, ComponentManager* componentManager) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Render UI elements in z-order (back to front)
    for (Entity entity : sortedUIElements) {
        if (!componentManager->hasComponent<UIComponent>(entity)) continue;

        auto& ui = componentManager->getComponent<UIComponent>(entity);
        if (ui.visible) {
            renderUIElement(entity, componentManager, renderer);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    metrics.renderTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

void UISystem::renderUIElement(Entity entity, ComponentManager* componentManager, SDL_Renderer* renderer) {
    auto& ui = componentManager->getComponent<UIComponent>(entity);

    switch (ui.type) {
        case UIElementType::PANEL:
            if (componentManager->hasComponent<UIPanelComponent>(entity)) {
                renderPanel(entity, ui, componentManager, renderer);
            }
            break;

        case UIElementType::BUTTON:
            if (componentManager->hasComponent<UIButtonComponent>(entity)) {
                auto& button = componentManager->getComponent<UIButtonComponent>(entity);
                renderButton(entity, ui, button, renderer);
            }
            break;

        case UIElementType::TEXT:
            if (componentManager->hasComponent<UITextComponent>(entity)) {
                auto& text = componentManager->getComponent<UITextComponent>(entity);
                renderText(entity, ui, text, renderer);
            }
            break;

        case UIElementType::SLIDER:
            if (componentManager->hasComponent<UISliderComponent>(entity)) {
                auto& slider = componentManager->getComponent<UISliderComponent>(entity);
                renderSlider(entity, ui, slider, renderer);
            }
            break;

        case UIElementType::INPUT_FIELD:
            if (componentManager->hasComponent<UIInputFieldComponent>(entity)) {
                auto& input = componentManager->getComponent<UIInputFieldComponent>(entity);
                renderInputField(entity, ui, input, renderer);
            }
            break;

        case UIElementType::IMAGE:
            if (componentManager->hasComponent<UIImageComponent>(entity)) {
                auto& image = componentManager->getComponent<UIImageComponent>(entity);
                renderImage(entity, ui, image, renderer);
            }
            break;
    }

    // Debug mode: draw bounding boxes
    if (debugMode) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 128);
        SDL_Rect debugRect = ui.getRect();
        SDL_RenderDrawRect(renderer, &debugRect);
    }
}

void UISystem::renderPanel(Entity entity, const UIComponent& ui, ComponentManager* componentManager, SDL_Renderer* renderer) {
    SDL_Rect rect = ui.getRect();
    SDL_Color bgColor = ui.getCurrentBackgroundColor();

    // Draw background
    drawRectangle(renderer, rect, bgColor, true);

    // Draw border
    if (ui.style.borderWidth > 0) {
        drawRectangle(renderer, rect, ui.style.borderColor, false);
    }
}

void UISystem::renderButton(Entity entity, const UIComponent& ui, const UIButtonComponent& button, SDL_Renderer* renderer) {
    SDL_Rect rect = ui.getRect();
    SDL_Color bgColor = ui.getCurrentBackgroundColor();

    // Draw button background
    if (ui.style.cornerRadius > 0) {
        drawRoundedRectangle(renderer, rect, ui.style.cornerRadius, bgColor);
    } else {
        drawRectangle(renderer, rect, bgColor, true);
    }

    // Draw border
    if (ui.style.borderWidth > 0) {
        drawRectangle(renderer, rect, ui.style.borderColor, false);
    }

    // Draw button text
    if (!button.text.empty()) {
        int textX = rect.x + (rect.w / 2);
        int textY = rect.y + (rect.h / 2);
        drawText(renderer, button.text, textX, textY, ui.style.textColor, ui.style.fontFamily);
    }
}

void UISystem::renderText(Entity entity, const UIComponent& ui, const UITextComponent& text, SDL_Renderer* renderer) {
    if (!text.text.empty()) {
        int textX = static_cast<int>(ui.absoluteX + ui.style.paddingLeft);
        int textY = static_cast<int>(ui.absoluteY + ui.style.paddingTop);
        drawText(renderer, text.text, textX, textY, ui.style.textColor, ui.style.fontFamily);
    }
}

void UISystem::renderSlider(Entity entity, const UIComponent& ui, const UISliderComponent& slider, SDL_Renderer* renderer) {
    SDL_Rect rect = ui.getRect();

    // Draw slider track
    SDL_Rect trackRect = {rect.x, rect.y + rect.h / 2 - 2, rect.w, 4};
    drawRectangle(renderer, trackRect, ui.style.borderColor, true);

    // Draw slider handle
    float handleX = rect.x + (slider.getNormalizedValue() * (rect.w - 20));
    SDL_Rect handleRect = {static_cast<int>(handleX), rect.y + rect.h / 2 - 10, 20, 20};
    drawRectangle(renderer, handleRect, ui.getCurrentBackgroundColor(), true);
    drawRectangle(renderer, handleRect, ui.style.borderColor, false);
}

void UISystem::renderInputField(Entity entity, const UIComponent& ui, const UIInputFieldComponent& input, SDL_Renderer* renderer) {
    SDL_Rect rect = ui.getRect();

    // Draw background
    drawRectangle(renderer, rect, ui.getCurrentBackgroundColor(), true);
    drawRectangle(renderer, rect, ui.style.borderColor, false);

    // Draw text or placeholder
    std::string displayText = input.text.empty() ? input.placeholder : input.text;
    SDL_Color textColor = input.text.empty() ?
        SDL_Color{128, 128, 128, 255} : ui.style.textColor;

    if (!displayText.empty()) {
        int textX = rect.x + ui.style.paddingLeft;
        int textY = rect.y + ui.style.paddingTop;
        drawText(renderer, displayText, textX, textY, textColor, ui.style.fontFamily);
    }

    // Draw cursor if focused
    if (input.focused && input.showCursor && !input.text.empty()) {
        // Calculate cursor position (simplified)
        int cursorX = rect.x + ui.style.paddingLeft + (input.cursorPosition * 8); // Approximate
        SDL_SetRenderDrawColor(renderer, ui.style.textColor.r, ui.style.textColor.g,
                              ui.style.textColor.b, ui.style.textColor.a);
        SDL_RenderDrawLine(renderer, cursorX, rect.y + 2, cursorX, rect.y + rect.h - 2);
    }
}

void UISystem::renderImage(Entity entity, const UIComponent& ui, const UIImageComponent& image, SDL_Renderer* renderer) {
    if (image.textureId.empty()) return;

    SDL_Texture* texture = assetManager.getTexture(image.textureId);
    if (!texture) return;

    SDL_Rect destRect = ui.getRect();

    if (image.preserveAspectRatio) {
        // Calculate aspect ratio preserving dimensions
        int texWidth, texHeight;
        SDL_QueryTexture(texture, nullptr, nullptr, &texWidth, &texHeight);

        float aspectRatio = static_cast<float>(texWidth) / texHeight;
        float uiAspectRatio = ui.width / ui.height;

        if (aspectRatio > uiAspectRatio) {
            // Texture is wider - fit to width
            int newHeight = static_cast<int>(ui.width / aspectRatio);
            destRect.y += (destRect.h - newHeight) / 2;
            destRect.h = newHeight;
        } else {
            // Texture is taller - fit to height
            int newWidth = static_cast<int>(ui.height * aspectRatio);
            destRect.x += (destRect.w - newWidth) / 2;
            destRect.w = newWidth;
        }
    }

    SDL_RenderCopy(renderer, texture, nullptr, &destRect);
}

void UISystem::drawRectangle(SDL_Renderer* renderer, const SDL_Rect& rect, const SDL_Color& color, bool filled) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    if (filled) {
        SDL_RenderFillRect(renderer, &rect);
    } else {
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void UISystem::drawRoundedRectangle(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, const SDL_Color& color) {
    // Simplified rounded rectangle (just draw regular rectangle for now)
    drawRectangle(renderer, rect, color, true);
}

void UISystem::drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, const SDL_Color& color, const std::string& fontId) {
    SDL_Texture* textTexture = getTextTexture(text, color, fontId);
    if (!textTexture) return;

    int textWidth, textHeight;
    SDL_QueryTexture(textTexture, nullptr, nullptr, &textWidth, &textHeight);

    SDL_Rect destRect = {x - textWidth / 2, y - textHeight / 2, textWidth, textHeight};
    SDL_RenderCopy(renderer, textTexture, nullptr, &destRect);
}

SDL_Texture* UISystem::getTextTexture(const std::string& text, const SDL_Color& color, const std::string& fontId) {
    std::string cacheKey = getTextCacheKey(text, color, fontId);

    auto it = textCache.find(cacheKey);
    if (it != textCache.end()) {
        return it->second;
    }

    // Create new text texture
    TTF_Font* font = assetManager.getFont(fontId);
    if (!font) return nullptr;

    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!textSurface) return nullptr;

    // We need to get the renderer from somewhere - for now, return nullptr
    // In a real implementation, we'd pass the renderer to the UISystem
    SDL_Texture* textTexture = nullptr; // SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    if (textTexture) {
        textCache[cacheKey] = textTexture;
    }

    return textTexture;
}

std::string UISystem::getTextCacheKey(const std::string& text, const SDL_Color& color, const std::string& fontId) {
    return text + "_" + std::to_string(color.r) + "_" + std::to_string(color.g) + "_" +
           std::to_string(color.b) + "_" + std::to_string(color.a) + "_" + fontId;
}

// UI Element Creation Methods
Entity UISystem::createButton(ComponentManager* componentManager, EntityManager* entityManager,
                             const std::string& text, float x, float y, float width, float height) {
    Entity entity = entityManager->createEntity();

    UIComponent ui(UIElementType::BUTTON);
    ui.x = x;
    ui.y = y;
    ui.width = width;
    ui.height = height;
    ui.interactive = true;
    ui.focusable = true;
    componentManager->addComponent(entity, ui);

    UIButtonComponent button(text);
    componentManager->addComponent(entity, button);

    return entity;
}

Entity UISystem::createText(ComponentManager* componentManager, EntityManager* entityManager,
                           const std::string& text, float x, float y) {
    Entity entity = entityManager->createEntity();

    UIComponent ui(UIElementType::TEXT);
    ui.x = x;
    ui.y = y;
    ui.width = 200.0f; // Auto-size would calculate this
    ui.height = 30.0f;
    ui.interactive = false;
    componentManager->addComponent(entity, ui);

    UITextComponent textComp(text);
    componentManager->addComponent(entity, textComp);

    return entity;
}

Entity UISystem::createPanel(ComponentManager* componentManager, EntityManager* entityManager,
                            float x, float y, float width, float height) {
    Entity entity = entityManager->createEntity();

    UIComponent ui(UIElementType::PANEL);
    ui.x = x;
    ui.y = y;
    ui.width = width;
    ui.height = height;
    ui.interactive = false;
    componentManager->addComponent(entity, ui);

    UIPanelComponent panel;
    componentManager->addComponent(entity, panel);

    return entity;
}

Entity UISystem::createSlider(ComponentManager* componentManager, EntityManager* entityManager,
                             float min, float max, float value, float x, float y, float width, float height) {
    Entity entity = entityManager->createEntity();

    UIComponent ui(UIElementType::SLIDER);
    ui.x = x;
    ui.y = y;
    ui.width = width;
    ui.height = height;
    ui.interactive = true;
    ui.focusable = true;
    componentManager->addComponent(entity, ui);

    UISliderComponent slider(min, max, value);
    componentManager->addComponent(entity, slider);

    return entity;
}

Entity UISystem::createInputField(ComponentManager* componentManager, EntityManager* entityManager,
                                 const std::string& placeholder, float x, float y, float width, float height) {
    Entity entity = entityManager->createEntity();

    UIComponent ui(UIElementType::INPUT_FIELD);
    ui.x = x;
    ui.y = y;
    ui.width = width;
    ui.height = height;
    ui.interactive = true;
    ui.focusable = true;
    componentManager->addComponent(entity, ui);

    UIInputFieldComponent input(placeholder);
    componentManager->addComponent(entity, input);

    return entity;
}

Entity UISystem::createImage(ComponentManager* componentManager, EntityManager* entityManager,
                            const std::string& textureId, float x, float y, float width, float height) {
    Entity entity = entityManager->createEntity();

    UIComponent ui(UIElementType::IMAGE);
    ui.x = x;
    ui.y = y;
    ui.width = width;
    ui.height = height;
    ui.interactive = false;
    componentManager->addComponent(entity, ui);

    UIImageComponent image(textureId);
    componentManager->addComponent(entity, image);

    return entity;
}

// Input Handling
void UISystem::handleInput(ComponentManager* componentManager, const SDL_Event& event) {
    metrics.eventHandles++;

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEMOTION:
            handleMouseEvent(componentManager, event);
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_TEXTINPUT:
            handleKeyboardEvent(componentManager, event);
            break;
    }
}

void UISystem::handleMouseEvent(ComponentManager* componentManager, const SDL_Event& event) {
    float mouseX = static_cast<float>(event.button.x);
    float mouseY = static_cast<float>(event.button.y);

    if (event.type == SDL_MOUSEMOTION) {
        mouseX = static_cast<float>(event.motion.x);
        mouseY = static_cast<float>(event.motion.y);
        updateHoverStates(mouseX, mouseY, componentManager);
        return;
    }

    Entity clickedEntity = findUIElementAt(mouseX, mouseY, componentManager);

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        if (clickedEntity != NO_ENTITY) {
            auto& ui = componentManager->getComponent<UIComponent>(clickedEntity);
            if (ui.interactive) {
                ui.state = UIState::PRESSED;
                pressedEntity = clickedEntity;

                // Set focus
                if (ui.focusable) {
                    setFocus(clickedEntity, componentManager);
                }

                // Handle specific element types
                if (ui.type == UIElementType::BUTTON && componentManager->hasComponent<UIButtonComponent>(clickedEntity)) {
                    auto& button = componentManager->getComponent<UIButtonComponent>(clickedEntity);
                    button.pressed = true;
                }
            }
        } else {
            // Clicked outside any UI element - clear focus
            setFocus(NO_ENTITY, componentManager);
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (pressedEntity != NO_ENTITY && componentManager->hasComponent<UIComponent>(pressedEntity)) {
            auto& ui = componentManager->getComponent<UIComponent>(pressedEntity);
            ui.state = (pressedEntity == clickedEntity) ? UIState::HOVERED : UIState::NORMAL;

            // Trigger click event if released on same element
            if (pressedEntity == clickedEntity && ui.interactive) {
                if (ui.onClicked) {
                    triggerCallback(pressedEntity, ui.onClicked);
                }
                generateUIEvent(pressedEntity, "ui_clicked", componentManager);

                // Handle specific element types
                if (ui.type == UIElementType::BUTTON && componentManager->hasComponent<UIButtonComponent>(pressedEntity)) {
                    auto& button = componentManager->getComponent<UIButtonComponent>(pressedEntity);
                    button.pressed = false;
                    button.wasPressed = true; // Mark for one-frame detection
                }
            }

            pressedEntity = NO_ENTITY;
        }
    }
}

void UISystem::handleKeyboardEvent(ComponentManager* componentManager, const SDL_Event& event) {
    if (focusedEntity == NO_ENTITY) return;

    if (!componentManager->hasComponent<UIComponent>(focusedEntity)) return;

    auto& ui = componentManager->getComponent<UIComponent>(focusedEntity);

    // Handle input field text input
    if (ui.type == UIElementType::INPUT_FIELD && componentManager->hasComponent<UIInputFieldComponent>(focusedEntity)) {
        auto& input = componentManager->getComponent<UIInputFieldComponent>(focusedEntity);

        if (event.type == SDL_TEXTINPUT) {
            input.insertText(event.text.text);
            if (input.onTextChanged) {
                input.onTextChanged(focusedEntity, input.text);
            }
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_BACKSPACE:
                    input.deleteCharacter();
                    if (input.onTextChanged) {
                        input.onTextChanged(focusedEntity, input.text);
                    }
                    break;
                case SDLK_LEFT:
                    input.moveCursor(-1);
                    break;
                case SDLK_RIGHT:
                    input.moveCursor(1);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (input.onEnterPressed) {
                        input.onEnterPressed(focusedEntity);
                    }
                    generateUIEvent(focusedEntity, "ui_enter_pressed", componentManager);
                    break;
            }
        }
    }
}

Entity UISystem::findUIElementAt(float x, float y, ComponentManager* componentManager) {
    // Search in reverse z-order (front to back)
    for (auto it = sortedUIElements.rbegin(); it != sortedUIElements.rend(); ++it) {
        Entity entity = *it;
        if (!componentManager->hasComponent<UIComponent>(entity)) continue;

        auto& ui = componentManager->getComponent<UIComponent>(entity);
        if (ui.visible && ui.interactive && ui.containsPoint(x, y)) {
            return entity;
        }
    }
    return NO_ENTITY;
}

void UISystem::updateHoverStates(float mouseX, float mouseY, ComponentManager* componentManager) {
    Entity newHoveredEntity = findUIElementAt(mouseX, mouseY, componentManager);

    // Clear previous hover state
    if (hoveredEntity != NO_ENTITY && componentManager->hasComponent<UIComponent>(hoveredEntity)) {
        auto& prevUI = componentManager->getComponent<UIComponent>(hoveredEntity);
        if (prevUI.state == UIState::HOVERED) {
            prevUI.state = UIState::NORMAL;
        }
    }

    // Set new hover state
    if (newHoveredEntity != NO_ENTITY && componentManager->hasComponent<UIComponent>(newHoveredEntity)) {
        auto& ui = componentManager->getComponent<UIComponent>(newHoveredEntity);
        if (ui.state != UIState::PRESSED) {
            ui.state = UIState::HOVERED;
        }

        if (ui.onHover && newHoveredEntity != hoveredEntity) {
            triggerCallback(newHoveredEntity, ui.onHover);
        }
    }

    hoveredEntity = newHoveredEntity;
}

void UISystem::setFocus(Entity entity, ComponentManager* componentManager) {
    // Clear previous focus
    if (focusedEntity != NO_ENTITY && componentManager->hasComponent<UIComponent>(focusedEntity)) {
        auto& prevUI = componentManager->getComponent<UIComponent>(focusedEntity);
        if (prevUI.onBlur) {
            triggerCallback(focusedEntity, prevUI.onBlur);
        }

        // Clear input field focus
        if (prevUI.type == UIElementType::INPUT_FIELD && componentManager->hasComponent<UIInputFieldComponent>(focusedEntity)) {
            auto& input = componentManager->getComponent<UIInputFieldComponent>(focusedEntity);
            input.focused = false;
        }
    }

    // Set new focus
    focusedEntity = entity;
    if (entity != NO_ENTITY && componentManager->hasComponent<UIComponent>(entity)) {
        auto& ui = componentManager->getComponent<UIComponent>(entity);
        if (ui.onFocus) {
            triggerCallback(entity, ui.onFocus);
        }

        // Set input field focus
        if (ui.type == UIElementType::INPUT_FIELD && componentManager->hasComponent<UIInputFieldComponent>(entity)) {
            auto& input = componentManager->getComponent<UIInputFieldComponent>(entity);
            input.focused = true;
            input.cursorPosition = input.text.length();
        }
    }
}

void UISystem::generateUIEvent(Entity entity, const std::string& eventName, ComponentManager* componentManager) {
    if (componentManager->hasComponent<EventComponent>(entity)) {
        auto& eventComp = componentManager->getComponent<EventComponent>(entity);
        eventComp.sendCustomEvent(eventName);
    }
}

void UISystem::triggerCallback(Entity entity, UIEventCallback callback) {
    if (callback) {
        try {
            callback(entity);
        } catch (const std::exception& e) {
            std::cerr << "[UISystem] Error in UI callback: " << e.what() << std::endl;
        }
    }
}

void UISystem::resetMetrics() {
    metrics = PerformanceMetrics{};
}
