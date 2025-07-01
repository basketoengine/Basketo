#include "Game.h"

int main(int argc, char* argv[]) {
    Game game;
    if (!game.init("Basketo-Game-Engine", 800, 600)) {
        return 1;
    }

    while (game.isRunning()) {
        Uint32 frameStart = SDL_GetTicks();
        
        game.handleEvents();
        game.update();
        game.render();

        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < game.frameDelay) {
            SDL_Delay(game.frameDelay - frameTime);
        }
    }

    return 0;
}
