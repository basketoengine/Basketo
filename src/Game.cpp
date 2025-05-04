#include "Game.h"
#include <iostream>
#include "InputManager.h"
#include "Physics.h"
#include "SceneManager.h"
#include "./scenes/GameScene.h"
#include "./scenes/MenuScene.h"
#include "./scenes/DevModeScene.h"
#include "AssetManager.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_sdl2.h"  
#include "../vendor/imgui/backends/imgui_impl_sdlrenderer2.h" 

Game::Game() {}

Game::~Game() {
    clean();
}

bool Game::init(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height, SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    AssetManager::getInstance().init(renderer);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Load Fonts (Optional - ImGui uses default if none loaded)
    // io.Fonts->AddFontFromFileTTF("../assets/fonts/roboto/Roboto-Regular.ttf", 16.0f);

    // Switch to DevModeScene initially for testing
    SceneManager::getInstance().changeScene(std::make_unique<DevModeScene>(renderer, window));
    //SceneManager::getInstance().changeScene(std::make_unique<MenuScene>(renderer));

    running = true;
    return true;
}

void Game::handleEvents() {
    SDL_Event event;
    InputManager::getInstance().update(); // Update InputManager state first

    while (SDL_PollEvent(&event)) {
        // Pass SDL events to ImGui FIRST
        ImGui_ImplSDL2_ProcessEvent(&event);

        // Pass event to the active scene's handleInput
        Scene* activeScene = SceneManager::getInstance().getActiveScene();
        if (activeScene) {
            activeScene->handleInput(event); // Pass the event object
        }

        // Handle your specific events *after* ImGui and the scene potentially consume them
        ImGuiIO& io = ImGui::GetIO();
        // Only process game-level quit/escape if ImGui is NOT capturing keyboard
        if (!io.WantCaptureKeyboard) {
             if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                 running = false;
             }
        }
        // Note: Scene handles mouse events if io.WantCaptureMouse is false
    }
}

void Game::update() {
    Uint32 currentFrameTime = SDL_GetTicks();
    deltaTime = (currentFrameTime - lastFrameTime) / 1000.0f;
    lastFrameTime = currentFrameTime;

    Scene* current = SceneManager::getInstance().getActiveScene();
    if (current) {
        // Remove handleInput call from update, it's now handled per-event in handleEvents
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
    // Cleanup ImGui
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Clean up assets *before* destroying the renderer
    AssetManager::getInstance().cleanup();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Quit SDL subsystems
    Mix_CloseAudio(); // Close mixer audio
    Mix_Quit();       // Quit SDL_mixer
    TTF_Quit();       // Quit SDL_ttf
    IMG_Quit();       // Quit SDL_image
    SDL_Quit();
}

bool Game::isRunning() const {
    return running;
}
