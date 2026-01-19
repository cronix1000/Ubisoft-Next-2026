#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h"
#include "SquadComponent.h"
#include "SquadMemberComponent.h"
#include "TransformComponent.h"
#include "UnitComponent.h"
#include "ThreeDVisualiser.h"
#include "BuilderComponent.h"
#include "ColliderComponent.h"

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
                // Check if near any player faction entity or factory
                bool nearPlayerFaction = IsNearPlayerFactionOrFactory(trans.Pos, 20.0f);
                vec3d velocity = { 0,0,0 };
                if (nearPlayerFaction)
                {
                    // Chase mode - move towards player
                    float dist = Engine3D::Vector_Distance(trans.Pos, playerPos);

                    // Stop if we are close enough (attack range)
                    if (dist > 5.0f)
                    {
                        vec3d dir = Engine3D::Vector_Normalise(Engine3D::Vector_Sub(playerPos, trans.Pos));
                        velocity = Engine3D::Vector_Mul(dir, squad.moveSpeed * (dt / 1000.0f));
                        trans.Pos = Engine3D::Vector_Add(trans.Pos, velocity);
                    }
                }
                else
                {
                    // Wander mode - move squad to random positions
                    squad.wanderTimer -= dt;
                    
                    if (squad.wanderTimer <= 0.0f)
                    {
                        // Pick new random wander target
                        float wanderRadius = 15.0f;
                        float randomAngle = (rand() % 360) * (PI / 180.0f);
                        float randomDist = (rand() % 100) / 100.0f * wanderRadius;
                        
                        squad.wanderTarget = {
                            trans.Pos.x + randomDist * cosf(randomAngle),
                            trans.Pos.y,
                            trans.Pos.z + randomDist * sinf(randomAngle)
                        };
                        
                        // Set new timer (3-6 seconds)
                        squad.wanderTimer = 3000.0f + (rand() % 3000);
                    }
                    
                    // Move towards wander target
                    vec3d diff = Engine3D::Vector_Sub(squad.wanderTarget, trans.Pos);
                    float dist = sqrtf(diff.x * diff.x + diff.z * diff.z);
                    
                    if (dist > 1.0f)
                    {
                        vec3d dir = Engine3D::Vector_Normalise(diff);
                        velocity = Engine3D::Vector_Mul(dir, squad.moveSpeed * 0.5f * (dt / 1000.0f));
                        trans.Pos = Engine3D::Vector_Add(trans.Pos, velocity);
                    }
                }

                vec3d nextPos = Engine3D::Vector_Add(trans.Pos, velocity);

                // --- CLAMP POSITION ---
                if (nextPos.x > MAP_LIMIT) nextPos.x = MAP_LIMIT;
                if (nextPos.x < -MAP_LIMIT) nextPos.x = -MAP_LIMIT;
                if (nextPos.z > MAP_LIMIT) nextPos.z = MAP_LIMIT;
                if (nextPos.z < -MAP_LIMIT) nextPos.z = -MAP_LIMIT;
                // ----------------------

                trans.Pos = nextPos;
            }
            // --- PLAYER SQUAD: CENTER ON PLAYER ---
            else if (squad.teamId == 0)
            {
               
                if (entity == GetPlayerSquadLeader())
                {
                    trans.Pos = playerPos;
                }
            }
        }

        // --- 2. UPDATE SQUAD MEMBERS (Formation) ---
        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<SquadMemberComponent>(entity)) continue;

            auto& member = gCoordinator.GetComponent<SquadMemberComponent>(entity);
            auto& unit = gCoordinator.GetComponent<UnitComponent>(entity);
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);

            // Get Leader Position
            if (!gCoordinator.HasComponent<TransformComponent>(member.squadId)) continue;
            auto& leaderTrans = gCoordinator.GetComponent<TransformComponent>(member.squadId);

            // Determine team
            int myTeam = -1;
            if (gCoordinator.HasComponent<FactionComponent>(entity))
            {
                myTeam = gCoordinator.GetComponent<FactionComponent>(entity).teamId;
            }

            // Only enemy units (team 1) should engage enemies
            if (myTeam == 1)
            {
                // Check if near any player faction entity or factory
                bool nearPlayerFaction = IsNearPlayerFactionOrFactory(trans.Pos, 20.0f);
                
                if (nearPlayerFaction)
                {
                    // Find and engage nearest enemy
                    Entity nearestEnemy = FindNearestEnemy(trans.Pos, myTeam);
                    
                    if (nearestEnemy != static_cast<Entity>(-1))
                    {
                        unit.targetEntity = nearestEnemy;
                        if (gCoordinator.HasComponent<TransformComponent>(nearestEnemy))
                        {
                            vec3d enemyPos = gCoordinator.GetComponent<TransformComponent>(nearestEnemy).Pos;

                            unit.targetPos = Engine3D::Vector_Add(enemyPos, member.formationOffset);
                            if (unit.targetPos.x > MAP_LIMIT) unit.targetPos.x = MAP_LIMIT;
                            if (unit.targetPos.x < -MAP_LIMIT) unit.targetPos.x = -MAP_LIMIT;
                            if (unit.targetPos.z > MAP_LIMIT) unit.targetPos.z = MAP_LIMIT;
                            if (unit.targetPos.z < -MAP_LIMIT) unit.targetPos.z = -MAP_LIMIT;

                            unit.isMoving = true;
                        }
                    }
                    else
                    {
                        // No enemy found - return to formation
                        unit.targetEntity = static_cast<Entity>(-1);
                        unit.targetPos = Engine3D::Vector_Add(leaderTrans.Pos, member.formationOffset);
                        unit.isMoving = true;
                    }
                }
                else
                {
                    // Not near player faction - maintain formation while squad wanders
                    unit.targetEntity = static_cast<Entity>(-1);
                    unit.targetPos = Engine3D::Vector_Add(leaderTrans.Pos, member.formationOffset);
                    unit.isMoving = true;
                }
            }
            else
            {
              
                unit.targetPos = Engine3D::Vector_Add(leaderTrans.Pos, member.formationOffset);
                unit.isMoving = true;

  
                Entity nearestEnemy = FindNearestEnemy(trans.Pos, myTeam);

                if (nearestEnemy != static_cast<Entity>(-1))
                {
                    unit.targetEntity = nearestEnemy;
                }
                else
                {
                    unit.targetEntity = static_cast<Entity>(-1);
                }
            }
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
        return static_cast<Entity>(-1);
    }

    bool IsNearPlayerFactionOrFactory(vec3d myPos, float maxRange)
    {
        float maxRangeSq = maxRange * maxRange;

        // Check all entities for player faction or factories
        for (auto const& targetEntity : mEntities)
        {
            if (!gCoordinator.HasComponent<TransformComponent>(targetEntity)) continue;
            auto& tPos = gCoordinator.GetComponent<TransformComponent>(targetEntity).Pos;
            float distSq = (tPos.x - myPos.x) * (tPos.x - myPos.x) + (tPos.z - myPos.z) * (tPos.z - myPos.z);

            if (distSq <= maxRangeSq)
            {
                // Check if it's player faction (team 0)
                if (gCoordinator.HasComponent<FactionComponent>(targetEntity))
                {
                    auto& faction = gCoordinator.GetComponent<FactionComponent>(targetEntity);
                    if (faction.teamId == 0) return true;
                }

                // Check if it's a factory with collider
                if (gCoordinator.HasComponent<BuilderComponent>(targetEntity) &&
                    gCoordinator.HasComponent<ColliderComponent>(targetEntity))
                {
                    return true;
                }
            }
        }
        return false;
    }

    Entity FindNearestEnemy(vec3d myPos, int myTeam)
    {
        Entity nearest = static_cast<Entity>(-1);
        float minDstSq = 20.0f * 20.0f; // Max Search Range

        // Iterate all entities to find enemies
        // (In optimized engine, use a Spatial Partition or specific list)
        for (auto const& targetEntity : mEntities)
        {
            if (!gCoordinator.HasComponent<FactionComponent>(targetEntity)) continue;
            auto& targetFaction = gCoordinator.GetComponent<FactionComponent>(targetEntity);

            if (targetFaction.teamId != myTeam)
            {
                auto& tPos = gCoordinator.GetComponent<TransformComponent>(targetEntity).Pos;
                float distSq = (tPos.x - myPos.x) * (tPos.x - myPos.x) + (tPos.z - myPos.z) * (tPos.z - myPos.z);

                if (distSq < minDstSq) {
                    minDstSq = distSq;
                    nearest = targetEntity;
                }
            }
        }
        return nearest;
    }
};