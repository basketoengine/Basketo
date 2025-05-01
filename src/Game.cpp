#include "Game.h"
#include <iostream>
#include "../utils/utility.h"
#include <SDL2/SDL_image.h>
#include "InputManager.h"
#include "Physics.h"

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
    IMG_Init(IMG_INIT_PNG);

    player = new Entity(100, 100, 50, 50);

    InputManager::getInstance().mapAction("MoveLeft", SDL_SCANCODE_A);
    InputManager::getInstance().mapAction("MoveRight", SDL_SCANCODE_D);
    InputManager::getInstance().mapAction("MoveUp", SDL_SCANCODE_W);
    InputManager::getInstance().mapAction("MoveDown", SDL_SCANCODE_S);

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

    InputManager::getInstance().update();
}

void Game::update() {
    Uint32 currentFrameTime = SDL_GetTicks();
    deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f;
    lastFrameTime = currentFrameTime;

    float speed = 200.0f;
    float vx = 0.0f;
    float vy = 0.0f;

    if (InputManager::getInstance().isActionPressed("MoveLeft"))  vx -= speed;
    if (InputManager::getInstance().isActionPressed("MoveRight")) vx += speed;
    if (InputManager::getInstance().isActionPressed("MoveUp"))    vy -= speed;
    if (InputManager::getInstance().isActionPressed("MoveDown"))  vy += speed;

    SDL_Rect prevPos = player->getRect(); // Save the player's current position

    player->setVelocity(vx, vy);
    player->update(deltaTime);

    // Check for collisions with the wall
    if (Physics::checkCollision(player->getRect(), wall.getRect())) {
        std::cout << "Collision detected!" << std::endl;
        player->setRect(prevPos); // Reset to the previous position
        player->setVelocity(0, 0); // Stop the player's movement
    }

    // Clamp the player's position within the window borders
    SDL_Rect playerRect = player->getRect();
    if (playerRect.x < 0) playerRect.x = 0; // Left border
    if (playerRect.y < 0) playerRect.y = 0; // Top border
    if (playerRect.x + playerRect.w > 800) playerRect.x = 800 - playerRect.w; // Right border
    if (playerRect.y + playerRect.h > 600) playerRect.y = 600 - playerRect.h; // Bottom border
    player->setRect(playerRect); // Update the player's position
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect = { 100, 100, 200, 150 };
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    drawCircle(renderer, 400, 300, 50);

    SDL_Texture* texture = loadTexture("../Image/logo.png");
    if (texture) {
        SDL_Rect destRect = { 300, 200, 100, 64 }; 
        SDL_RenderCopy(renderer, texture, nullptr, &destRect);
        SDL_DestroyTexture(texture);
    }

    player->render(renderer);\

    wall.render(renderer);


    SDL_RenderPresent(renderer);
}

SDL_Texture* Game::loadTexture(const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "IMG_Load Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    SDL_SetTextureColorMod(texture, 255, 255, 255);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

void Game::clean() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    IMG_Quit();
}

bool Game::isRunning() const {
    return running;
}
