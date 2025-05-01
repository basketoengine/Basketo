#pragma once
#include "../Scene.h"
#include "../Entity.h"
#include <SDL2/SDL.h>
#include <string>

class GameScene : public Scene {
public:
    GameScene(SDL_Renderer* renderer);
    void handleInput() override;
    void update(float deltaTime) override;
    void render() override;

private:
    SDL_Renderer* renderer;
    Entity* player;
    Entity wall = Entity(300, 100, 100, 100);
    SDL_Texture* loadTexture(const std::string& path);
};
