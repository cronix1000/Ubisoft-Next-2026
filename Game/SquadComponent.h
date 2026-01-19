#pragma once
#include "ThreeDVisualiser.h"
struct SquadComponent {
    int teamId;             // 0 = Player, 1 = Enemy
    vec3d currentTarget;    // Where the whole group is going
    float stateTimer;       // How long until we change orders?
    float radius;           // How spread out the squad is
    float moveSpeed = 5.0f;        // How fast the squad moves
    AIComponent::Type currentState = AIComponent::Type::Wander; // Current behavior state
    
    // Wander state
    vec3d wanderTarget = {0, 0, 0};
    float wanderTimer = 0.0f;
};