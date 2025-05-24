\
#pragma once
#include "../../vendor/nlohmann/json.hpp" 

struct CameraComponent {
    float width = 800.0f; 
    float height = 600.0f; 
    float zoom = 1.0f;
    // Potentially need to add position if the camera is not tied to an entity's transform
    // float x = 0.0f;
    // float y = 0.0f;
    bool isActive = false;

};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CameraComponent, width, height, zoom, isActive)
