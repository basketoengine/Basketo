#pragma once

struct TransformComponent {
    float x = 0.0f;
    float y = 0.0f;

    float scaleX = 1.0f;
    float scaleY = 1.0f;

    float width = 0.0f; // Add width
    float height = 0.0f; // Add height

    float rotation = 0.0f; // optional, not used yet
};
