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
    // Helper to recalculate offsets (Spiral/Circle)
    void RecalculateFormation(Entity leaderEntity)
    {
        int count = 0;
        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<SquadMemberComponent>(entity)) continue;
            auto& member = gCoordinator.GetComponent<SquadMemberComponent>(entity);

            if (member.squadId == leaderEntity)
            {
                // Golden Angle Spiral
                float spacing = .2f; // Distance between units
                float radius = spacing * sqrtf(count);
                float theta = count * 2.4f;

                member.formationOffset = { radius * cosf(theta), 0.0f, radius * sinf(theta) };
                count++;
            }
        }
    }

    void Update(float dt)
    {
        vec3d playerPos = { 0,0,0 };
        if (gCoordinator.HasComponent<TransformComponent>(playerUnit))
            playerPos = gCoordinator.GetComponent<TransformComponent>(playerUnit).Pos;

        // --- 1. UPDATE SQUAD LEADERS ---
        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<SquadComponent>(entity)) continue;

            auto& squad = gCoordinator.GetComponent<SquadComponent>(entity);
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);

            // --- ENEMY AI: CHASE PLAYER ---
            if (squad.teamId == 1)
            {
                // Move Leader towards Player
                float dist = Engine3D::Vector_Distance(trans.Pos, playerPos);

                // Stop if we are close enough (attack range)
                if (dist > 5.0f)
                {
                    vec3d dir = Engine3D::Vector_Normalise(Engine3D::Vector_Sub(playerPos, trans.Pos));
                    vec3d velocity = Engine3D::Vector_Mul(dir, squad.moveSpeed * (dt / 1000.0f));
                    trans.Pos = Engine3D::Vector_Add(trans.Pos, velocity);
                }
            }
            // --- PLAYER SQUAD: CENTER ON PLAYER ---
            else if (squad.teamId == 0)
            {
                // If this is the main squad, SNAP the leader position to the Player Unit
                // This ensures the squad is always "in the middle" (surrounding the player)
                // We check dist to avoid jitter, or just hard set it.
                if (entity == GetPlayerSquadLeader())
                {
                    trans.Pos = playerPos;
                }
                // If it's a split-off squad, it stays put (or moves to clicked location logic if you add it)
            }
        }

        // --- 2. UPDATE SQUAD MEMBERS (Formation) ---
        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<SquadMemberComponent>(entity)) continue;

            auto& member = gCoordinator.GetComponent<SquadMemberComponent>(entity);
            auto& unit = gCoordinator.GetComponent<UnitComponent>(entity);

            // Get Leader Position
            if (!gCoordinator.HasComponent<TransformComponent>(member.squadId)) continue;
            auto& leaderTrans = gCoordinator.GetComponent<TransformComponent>(member.squadId);

            // Target = LeaderPos + Offset
            unit.targetPos = Engine3D::Vector_Add(leaderTrans.Pos, member.formationOffset);
            unit.isMoving = true;
        }
    }

    // Helper to find which squad leader is currently attached to the player
    Entity GetPlayerSquadLeader()
    {
        for (auto const& entity : mEntities)
        {
            if (gCoordinator.HasComponent<SquadComponent>(entity))
            {
                auto& squad = gCoordinator.GetComponent<SquadComponent>(entity);
                if (squad.teamId == 0) return entity; // Assuming first one is main for now
            }
        }
        return -1;
    }
};