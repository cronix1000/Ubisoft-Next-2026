#pragma once
#include "ThreeDVisualiser.h" // For vec3d
#include "ECSBase.h" // For Entity

struct UnitComponent {
    enum class UnitType {
        meleeGrunt,
        Ranged,
        Catapult
    } type = UnitType::meleeGrunt;

    vec3d targetPos;  

    bool isMoving = false;
    float speed = 10.0f;  
    bool isSelected = true;
    Entity targetEntity = static_cast<Entity>(-1);
};