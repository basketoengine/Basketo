#include "Game.h"
#include <iostream>

Game::Game() {}

Game::~Game() {
    clean();
}

bool Game::init(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height, SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer Error: " << SDL_GetError() << std::endl;
        return false;
    }

    running = true;
    return true;
}

void Game::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || 
           (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
            running = false;
        }
    }
}

void Game::update() {
    // Game logic will go here
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    // Draw a rectangle
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect = { 100, 100, 200, 150 };
    SDL_RenderFillRect(renderer, &rect);

    SDL_RenderPresent(renderer);
}

void Game::clean() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Game::isRunning() const {
    return running;
}
