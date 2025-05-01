#pragma once
#include "../Scene.h"
#include <SDL2/SDL.h>

class MenuScene : public Scene {
public:
    MenuScene(SDL_Renderer* renderer);
    void handleInput() override;
    void update(float deltaTime) override;
    void render() override;

private:
    SDL_Renderer* renderer;
    bool switchToGame = false;
};
