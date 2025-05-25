#pragma once
#include "../../vendor/nlohmann/json.hpp"

struct RigidbodyComponent {
    float mass = 1.0f;
    bool useGravity = true; 
    bool isStatic = false;
    float gravityScale = 1.0f;
    float drag = 0.0f; 
    bool isKinematic = false; 

    // Define JSON serialization/deserialization
    friend void to_json(nlohmann::json& j, const RigidbodyComponent& rb) {
        j = nlohmann::json{{"mass", rb.mass}, {"useGravity", rb.useGravity}, {"isStatic", rb.isStatic}, {"gravityScale", rb.gravityScale}, {"drag", rb.drag}, {"isKinematic", rb.isKinematic}};
    }

    friend void from_json(const nlohmann::json& j, RigidbodyComponent& rb) {
        j.at("mass").get_to(rb.mass);
        j.at("useGravity").get_to(rb.useGravity);
        j.at("isStatic").get_to(rb.isStatic);
        j.at("gravityScale").get_to(rb.gravityScale);
        j.at("drag").get_to(rb.drag);
        j.at("isKinematic").get_to(rb.isKinematic);
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RigidbodyComponent, mass, useGravity, isStatic, gravityScale, drag, isKinematic)
