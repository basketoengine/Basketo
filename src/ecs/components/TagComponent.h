#pragma once

#include <string>
#include "../Component.h" 
#include "../../vendor/nlohmann/json.hpp"

struct TagComponent : public Component {
    std::string tag = "default";

    TagComponent() = default;
    TagComponent(const std::string& t) : tag(t) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TagComponent, tag)
};
