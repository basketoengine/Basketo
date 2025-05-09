#pragma once
#include <vector>
#include <utility>
#include <SDL2/SDL_rect.h>
#include "../ecs/components/TransformComponent.h"
#include "../scenes/DevModeScene.h" // For ResizeHandle enum

std::vector<std::pair<ResizeHandle, SDL_Rect>> getResizeHandles(const TransformComponent& transform);
