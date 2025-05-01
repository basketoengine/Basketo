#include "GameScene.h"
#include "../InputManager.h"
#include "../Physics.h"
#include "../../utils/utility.h"
#include <SDL2/SDL_image.h>
#include <iostream>

GameScene::GameScene(SDL_Renderer* ren) : renderer(ren) {
    player = new Entity(100, 100, 50, 50);
    InputManager::getInstance().mapAction("MoveLeft", SDL_SCANCODE_A);
    InputManager::getInstance().mapAction("MoveRight", SDL_SCANCODE_D);
    InputManager::getInstance().mapAction("MoveUp", SDL_SCANCODE_W);
    InputManager::getInstance().mapAction("MoveDown", SDL_SCANCODE_S);
}

GameScene::~GameScene() {
    delete player;
}

void GameScene::handleInput() {
    InputManager::getInstance().update();
}

void GameScene::update(float deltaTime) {
    float speed = 200.0f;
    float vx = 0.0f;
    float vy = 0.0f;

    if (InputManager::getInstance().isActionPressed("MoveLeft"))  vx -= speed;
    if (InputManager::getInstance().isActionPressed("MoveRight")) vx += speed;
    if (InputManager::getInstance().isActionPressed("MoveUp"))    vy -= speed;
    if (InputManager::getInstance().isActionPressed("MoveDown"))  vy += speed;

    SDL_Rect prevPos = player->getRect();

    player->setVelocity(vx, vy);
    player->update(deltaTime);

    if (Physics::checkCollision(player->getRect(), wall.getRect())) {
        player->setRect(prevPos);
        player->setVelocity(0, 0);
    }

    // Clamp the player's position within the window borders
    SDL_Rect playerRect = player->getRect();
    if (playerRect.x < 0) playerRect.x = 0;
    if (playerRect.y < 0) playerRect.y = 0;
    if (playerRect.x + playerRect.w > 800) playerRect.x = 800 - playerRect.w;
    if (playerRect.y + playerRect.h > 600) playerRect.y = 600 - playerRect.h;
    player->setRect(playerRect);
}

void GameScene::render() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect rect = { 100, 100, 200, 150 };
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    drawCircle(renderer, 400, 300, 50);

    SDL_Texture* texture = loadTexture("../assets/Image/logo.png");
    if (texture) {
        SDL_Rect destRect = { 300, 200, 100, 64 }; 
        SDL_RenderCopy(renderer, texture, nullptr, &destRect);
        SDL_DestroyTexture(texture);
    }

    player->render(renderer);\

    wall.render(renderer);


    SDL_RenderPresent(renderer);
}

SDL_Texture* GameScene::loadTexture(const std::string& path) {
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
