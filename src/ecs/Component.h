#pragma once

#include "Types.h"

struct Component {
    Entity owner; // Optional: if components need to know their owner entity
    virtual ~Component() = default;
};
