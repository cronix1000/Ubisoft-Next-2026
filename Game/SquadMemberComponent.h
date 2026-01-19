
#pragma once
#include "ECSBase.h"
#include "ThreeDVisualiser.h"
struct SquadMemberComponent {
    Entity squadId;        
    vec3d formationOffset;
    
    // Wandering state
    vec3d wanderTarget = {0, 0, 0};
    float wanderTimer = 0.0f;
};