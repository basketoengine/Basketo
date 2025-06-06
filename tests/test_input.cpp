#include "gtest/gtest.h"
#include "InputManager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>

TEST(InputTest, BasicInput) {
    InputManager input;
    SDL_Event event;
    
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_SPACE;
    input.handleEvent(event);
    EXPECT_TRUE(input.isKeyPressed(SDLK_SPACE));
}

TEST(InputTest, KeyState) {
    InputManager input;
    SDL_Event event;
    
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_A;
    input.handleEvent(event);
    EXPECT_TRUE(input.isKeyPressed(SDLK_A));
    
    event.type = SDL_KEYUP;
    event.key.keysym.sym = SDLK_A;
    input.handleEvent(event);
    EXPECT_FALSE(input.isKeyPressed(SDLK_A));
}

TEST(InputTest, MouseInput) {
    InputManager input;
    SDL_Event event;
    
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    input.handleEvent(event);
    EXPECT_TRUE(input.isMouseButtonPressed(SDL_BUTTON_LEFT));
}
