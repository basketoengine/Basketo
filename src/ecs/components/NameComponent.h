\
#pragma once
#include <string>
#include "../../../vendor/nlohmann/json.hpp" 

struct NameComponent {
    std::string name = "Entity";

    NameComponent() = default;
    NameComponent(const std::string& n) : name(n) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(NameComponent, name)
};
