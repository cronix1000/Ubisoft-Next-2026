#pragma once
#include "ThreeDVisualiser.h"
struct SquadComponent {
    int teamId;             // 0 = Player, 1 = Enemy
    vec3d currentTarget;    // Where the whole group is going
    float stateTimer;       // How long until we change orders?
    float radius;           // How spread out the squad is
};