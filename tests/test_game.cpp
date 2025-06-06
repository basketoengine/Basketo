#include "gtest/gtest.h"
#include "Game.h"
#include "SDL2/SDL.h"

TEST(GameTest, Initialization) {
    Game game;
    EXPECT_FALSE(game.init("Test", 800, 600));
    SDL_Init(SDL_INIT_VIDEO);
    EXPECT_TRUE(game.init("Test", 800, 600));
}

TEST(GameTest, FrameRate) {
    Game game;
    SDL_Init(SDL_INIT_VIDEO);
    game.init("Test", 800, 600);
    EXPECT_EQ(game.frameDelay, 16);
}

TEST(GameTest, WindowProperties) {
    Game game;
    SDL_Init(SDL_INIT_VIDEO);
    game.init("Test", 800, 600);
    SDL_Window* window = SDL_CreateWindow("Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    EXPECT_TRUE(window);
    SDL_DestroyWindow(window);
}
