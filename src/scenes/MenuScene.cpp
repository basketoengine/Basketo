#include "MenuScene.h"
#include "GameScene.h"
#include "../SceneManager.h"
#include "../InputManager.h"
#include <iostream>

MenuScene::MenuScene(SDL_Renderer* ren) : renderer(ren) {
    if (TTF_Init() < 0) {
        std::cerr << "Failed to init TTF: " << TTF_GetError() << "\n";
    }
    font = TTF_OpenFont("../assets/fonts/roboto/Roboto-Regular.ttf", 32);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << "\n";
    }

    InputManager::getInstance().mapAction("StartGame", SDL_SCANCODE_RETURN);
}

MenuScene::~MenuScene() {
    TTF_CloseFont(font);
    TTF_Quit();
}

void MenuScene::renderText(const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = { x, y, surface->w, surface->h };
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

void MenuScene::handleInput(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
        if (InputManager::getInstance().isActionPressed("StartGame")) {
             switchToGame = true;
        }
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

    SDL_Rect title = { 200, 100, 400, 100 };
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    SDL_RenderFillRect(renderer, &title);

    SDL_Rect prompt = { 250, 300, 300, 50 };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &prompt);

    SDL_Color white = {255, 255, 255, 255};
    renderText("My 2D Game Engine", 200, 100, white);
    renderText("Press ENTER to Start", 220, 300, white);

    SDL_RenderPresent(renderer);
}
