#pragma once
#include <cstdint>  
#include <bitset>  
#include <queue>  
#include <cassert>  
#include <array> // Include the array header to resolve the incomplete type error  

using Entity = std::uint32_t;

// Used to define the size of arrays later on  
const Entity MAX_ENTITIES = 10000;

using ComponentType = std::uint8_t;

const ComponentType MAX_COMPONENTS = 32;

using Signature = std::bitset<MAX_COMPONENTS>;