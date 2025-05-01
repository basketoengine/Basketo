#include "MenuScene.h"
#include "GameScene.h"
#include "../SceneManager.h"
#include "../InputManager.h"

MenuScene::MenuScene(SDL_Renderer* ren) : renderer(ren) {
    InputManager::getInstance().mapAction("StartGame", SDL_SCANCODE_RETURN);
}

void MenuScene::handleInput() {
    if (InputManager::getInstance().isActionPressed("StartGame")) {
        switchToGame = true;
    }
}

void MenuScene::update(float deltaTime) {
    if (switchToGame) {
        SceneManager::getInstance().changeScene(std::make_unique<GameScene>(renderer));
        switchToGame = false;
    }
}

void MenuScene::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    // Placeholder UI
    SDL_Rect title = { 200, 100, 400, 100 };
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    SDL_RenderFillRect(renderer, &title);

    SDL_Rect prompt = { 250, 300, 300, 50 };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &prompt);

    SDL_RenderPresent(renderer);
}
