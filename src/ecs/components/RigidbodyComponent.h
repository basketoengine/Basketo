#pragma once

struct RigidbodyComponent {
    float mass = 1.0f;
    bool affectedByGravity = true;
    bool isStatic = false;
    float gravityScale = 1.0f;
};
