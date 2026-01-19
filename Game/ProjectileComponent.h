#pragma once
#include "ThreeDVisualiser.h"

struct ProjectileComponent {
    vec3d velocity;
    int damage;
    int ownerTeamId; 
    float lifetime;  
};