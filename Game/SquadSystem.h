#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h"
#include "SquadComponent.h"
#include "SquadMemberComponent.h"

extern Coordinator gCoordinator;

class SquadSystem : public System
{
public:
    void Update(float dt)
    {
        for (auto const& entity : mEntities)
        {
            auto& squad = gCoordinator.GetComponent<SquadComponent>(entity);
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);

            // 1. Update Timer
            squad.stateTimer -= dt;

            // 2. Squad Logic (Simple Wander)
            // Every 5 seconds, pick a new random spot for the group
            if (squad.stateTimer <= 0)
            {
                float rX = ((rand() % 100) / 5.0f) - 10.0f; // -10 to 10
                float rZ = ((rand() % 100) / 5.0f) - 10.0f;
                
                // Set the Group's Target
                squad.currentTarget = { transform.Pos.x + rX, 0, transform.Pos.z + rZ };
                
                // Reset Timer
                squad.stateTimer = 5000.0f; 
            }

            // 3. Move the "Virtual Squad Center" towards the target
            // This is invisible, but the soldiers will follow it.
            vec3d dir = Engine3D::Vector_Sub(squad.currentTarget, transform.Pos);
            float dist = sqrt(dir.x*dir.x + dir.z*dir.z);
            
            if (dist > 0.1f) {
                vec3d norm = Engine3D::Vector_Div(dir, dist);
                float speed = 2.0f * (dt / 1000.0f); // Squad moves slower/smoother
                transform.Pos = Engine3D::Vector_Add(transform.Pos, Engine3D::Vector_Mul(norm, speed));
            }
        }
    }
};