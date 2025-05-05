#pragma once

struct RigidbodyComponent {
    float mass = 1.0f;
    bool useGravity = true; 
    bool isStatic = false;
    float gravityScale = 1.0f;
    float drag = 0.0f; 
    bool isKinematic = false; 
};
