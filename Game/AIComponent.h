#pragma once
#include "ThreeDVisualiser.h"

struct AIComponent {
    enum class Type {
        None,
        Wander,
        Chaser
    };

    Type type = Type::None;

    // State Data
    float actionTimer = 0.0f;      // For wandering delays
    float attackCooldown = 0.0f;   // Time until next shot
    vec3d wanderTarget = { 0,0,0 };  // Where am I going?
};