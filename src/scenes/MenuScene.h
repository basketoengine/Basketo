#pragma once
#include "../Scene.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>

class MenuScene : public Scene {
public:
    MenuScene(SDL_Renderer* renderer);
    ~MenuScene() override;
    void handleInput() override;
    void update(float deltaTime) override;
    void render() override;

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    bool switchToGame = false;

    void renderText(const std::string& text, int x, int y, SDL_Color color);
};
