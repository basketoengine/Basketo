#include "Game.h"
#include <iostream>
#include "InputManager.h"
#include "Physics.h"
#include "SceneManager.h"
#include "./scenes/GameScene.h"
#include "./scenes/MenuScene.h"
#include "AssetManager.h" // Include AssetManager
#include <SDL2/SDL_image.h>

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

    // Initialize SDL_image *before* AssetManager needs it
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        // Handle initialization error, maybe return false
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    // Initialize AssetManager with the renderer
    AssetManager::getInstance().init(renderer);

    //SceneManager::getInstance().changeScene(std::make_unique<GameScene>(renderer));
    SceneManager::getInstance().changeScene(std::make_unique<MenuScene>(renderer));

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

    InputManager::getInstance().update();
}

void Game::update() {
    Uint32 currentFrameTime = SDL_GetTicks();
    deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f;
    lastFrameTime = currentFrameTime;

    Scene* current = SceneManager::getInstance().getActiveScene();
    if (current) {
        current->handleInput();
        current->update(deltaTime);
    }
}

void Game::render() {
    Scene* current = SceneManager::getInstance().getActiveScene();
    if (current) {
        current->render();
    } else {
       
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

}

void Game::clean() {
    // Clean up assets *before* destroying the renderer
    AssetManager::getInstance().cleanup();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit(); // Quit SDL_image
    SDL_Quit();
}

bool Game::isRunning() const {
    return running;
}
