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
    float actionTimer = 0.0f;
    float attackCooldown = 0.0f;
    vec3d wanderTarget = { 0,0,0 };

    float attackRange = 4.0f;
    float separationRadius = 1.5f;

    bool isSelected = true;
    Entity targetEntity = static_cast<Entity>(-1);
};