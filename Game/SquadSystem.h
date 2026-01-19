#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h"
#include "SquadComponent.h"
#include "SquadMemberComponent.h"
#include "TransformComponent.h"
#include "UnitComponent.h"
#include "ThreeDVisualiser.h"

extern Coordinator gCoordinator;
extern Entity playerUnit; // Global reference to player

class SquadSystem : public System
{
public:
    void Update(float dt)
    {
        vec3d playerPos = {0,0,0};
        if (gCoordinator.HasComponent<TransformComponent>(playerUnit))
             playerPos = gCoordinator.GetComponent<TransformComponent>(playerUnit).Pos;

        // --- 1. UPDATE SQUAD LEADERS (The Brains) ---
        for (auto const& entity : mEntities)
        {
            // Filter for Squad Leaders
            if (!gCoordinator.HasComponent<SquadComponent>(entity)) continue;

            auto& squad = gCoordinator.GetComponent<SquadComponent>(entity);
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);

            // Simple AI: Move Squad Leader towards Player
            // (Units will follow this invisible point)
            vec3d dir = Engine3D::Vector_Normalise(Engine3D::Vector_Sub(playerPos, trans.Pos));
            
            // Stop if the SQUAD CENTER is close to player (prevents pushing too far)
            if (Engine3D::Vector_Distance(trans.Pos, playerPos) > 6.0f)
            {
                vec3d velocity = Engine3D::Vector_Mul(dir, squad.moveSpeed * (dt / 1000.0f));
                trans.Pos = Engine3D::Vector_Add(trans.Pos, velocity);
            }
        }

        // --- 2. UPDATE SQUAD MEMBERS (The Grunts) ---
        for (auto const& entity : mEntities)
        {
            // Filter for Members
            if (!gCoordinator.HasComponent<SquadMemberComponent>(entity)) continue;

            auto& member = gCoordinator.GetComponent<SquadMemberComponent>(entity);
            auto& unit   = gCoordinator.GetComponent<UnitComponent>(entity);

            // Get Leader Position
            // (Note: In production code, check if entity exists first)
            auto& leaderTrans = gCoordinator.GetComponent<TransformComponent>(member.squadId);

            // CALCULATE CLUMP POSITION
            // Target = LeaderPos + MyOffset
            unit.targetPos = Engine3D::Vector_Add(leaderTrans.Pos, member.formationOffset);
            
            // Default to moving (UnitSystem will override if attacking)
            unit.isMoving = true; 
        }
    }
};