#pragma once
#include "ThreeDVisualiser.h"

struct ProjectileComponent {
    vec3d velocity;
    int damage;
    int ownerTeamId; 
    float lifetime;
    float splashRadius = 0.0f; // AoE damage radius (0 = single target only)
};