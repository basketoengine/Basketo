#pragma once
#include "../../vendor/nlohmann/json.hpp"

struct RigidbodyComponent {
    float mass = 1.0f;
    bool useGravity = true; 
    bool isStatic = false;
    float gravityScale = 1.0f;
    float drag = 0.0f; 
    bool isKinematic = false; 
    bool isGrounded = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RigidbodyComponent, mass, useGravity, isStatic, gravityScale, drag, isKinematic, isGrounded) 
