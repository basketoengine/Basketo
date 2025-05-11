\
#pragma once

#include <SDL2/SDL_events.h>

class DevModeScene;

void handleDevModeInput(DevModeScene& scene, SDL_Event& event);

class DevModeInputHandler {
public:
    explicit DevModeInputHandler(DevModeScene& scene);


private:
    DevModeScene& m_sceneRef;
};
