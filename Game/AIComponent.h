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
    float actionTimer = 0.0f;     
    float attackCooldown = 0.0f;  
    vec3d wanderTarget = { 0,0,0 }; 

    float speed = 3.0f;
    float attackRange = 4.0f;    
    float separationRadius = 1.5f; 

 
    int state = 0;
};