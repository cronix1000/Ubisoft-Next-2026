#pragma once
#include "ThreeDVisualiser.h" // For vec3d

struct UnitComponent {
    vec3d targetPos;  
    bool isMoving = false;
    float speed = 10.0f;  
    bool isSelected = true; 
};