\
#pragma once
#include <string>

struct ColliderComponent {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float width = 0.0f; 
    float height = 0.0f; 

    std::string tag = "default";
    int layer = 0;

    ColliderComponent(float w = 0.0f, float h = 0.0f, float offX = 0.0f, float offY = 0.0f, std::string t = "default", int l = 0)
        : width(w), height(h), offsetX(offX), offsetY(offY), tag(t), layer(l) {}
};
