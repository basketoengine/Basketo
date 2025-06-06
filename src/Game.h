#pragma once
#include <SDL2/SDL.h>
#include "MemoryTracker.h"

class Game {
public:
    Game() {
        window = (SDL_Window*)MemoryTracker::allocate(sizeof(SDL_Window), __FILE__, __LINE__);
        renderer = (SDL_Renderer*)MemoryTracker::allocate(sizeof(SDL_Renderer), __FILE__, __LINE__);
    };
    ~Game() {
        MemoryTracker::deallocate(window);
        MemoryTracker::deallocate(renderer);
    };

    bool init(const char* title, int width, int height);
    void handleEvents();
    void update();
    void render();
    void clean();
    bool isRunning() const;
    Uint32 lastFrameTime = 0;
    float deltaTime = 0.0f;
    const int targetFPS = 60;
    const int frameDelay = 1000 / targetFPS;

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = false;
};
