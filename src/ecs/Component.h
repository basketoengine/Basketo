#pragma once

#include <cstdint>
#include <bitset>

const std::uint8_t MAX_COMPONENTS = 32;
using ComponentType = std::uint8_t;
using Signature = std::bitset<MAX_COMPONENTS>;
