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
private:
    // Spatial grid for fast enemy lookups
    static constexpr float GRID_CELL_SIZE = 10.0f;
    std::unordered_map<int, std::vector<Entity>> spatialGridTeam0;
    std::unordered_map<int, std::vector<Entity>> spatialGridTeam1;
    
    int GetGridKey(vec3d pos) {
        int x = static_cast<int>(pos.x / GRID_CELL_SIZE);
        int z = static_cast<int>(pos.z / GRID_CELL_SIZE);
        return (x << 16) | (z & 0xFFFF);
    }
    
    void RebuildSpatialGrid() {
        spatialGridTeam0.clear();
        spatialGridTeam1.clear();
        
        for (auto const& entity : mEntities) {
            if (!gCoordinator.HasComponent<FactionComponent>(entity)) continue;
            if (!gCoordinator.HasComponent<TransformComponent>(entity)) continue;
            
            auto& faction = gCoordinator.GetComponent<FactionComponent>(entity);
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);
            int key = GetGridKey(trans.Pos);
            
            if (faction.teamId == 0) {
                spatialGridTeam0[key].push_back(entity);
            } else if (faction.teamId == 1) {
                spatialGridTeam1[key].push_back(entity);
            }
        }
    }

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
        // Rebuild spatial grid once per frame
        RebuildSpatialGrid();
        
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
                bool nearPlayerFaction = IsNearPlayerFactionOrFactory(trans.Pos, 50.0f);
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
                bool nearPlayerFaction = IsNearPlayerFactionOrFactory(trans.Pos, 50.0f);
                
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
        int centerKey = GetGridKey(myPos);
        
        // Check only nearby grid cells (3x3 region)
        int centerX = static_cast<int>(myPos.x / GRID_CELL_SIZE);
        int centerZ = static_cast<int>(myPos.z / GRID_CELL_SIZE);
        
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                int key = ((centerX + dx) << 16) | ((centerZ + dz) & 0xFFFF);
                
                if (spatialGridTeam0.find(key) == spatialGridTeam0.end()) continue;
                
                for (Entity targetEntity : spatialGridTeam0[key]) {
                    if (!gCoordinator.HasComponent<TransformComponent>(targetEntity)) continue;
                    auto& tPos = gCoordinator.GetComponent<TransformComponent>(targetEntity).Pos;
                    float distSq = (tPos.x - myPos.x) * (tPos.x - myPos.x) + (tPos.z - myPos.z) * (tPos.z - myPos.z);
                    
                    if (distSq <= maxRangeSq) return true;
                }
            }
        }

        // Check all entities for factories (less common)
        for (auto const& targetEntity : mEntities)
        {
            if (!gCoordinator.HasComponent<TransformComponent>(targetEntity)) continue;
            auto& tPos = gCoordinator.GetComponent<TransformComponent>(targetEntity).Pos;
            float distSq = (tPos.x - myPos.x) * (tPos.x - myPos.x) + (tPos.z - myPos.z) * (tPos.z - myPos.z);

            if (distSq <= maxRangeSq)
            {
                // Check if it's a factory with collider
                if (gCoordinator.HasComponent<FactoryComponent>(targetEntity) &&
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
        
        // Use spatial grid for the opposite team
        auto& enemyGrid = (myTeam == 0) ? spatialGridTeam1 : spatialGridTeam0;
        
        // Check only nearby grid cells (5x5 region for 20 unit search range)
        int centerX = static_cast<int>(myPos.x / GRID_CELL_SIZE);
        int centerZ = static_cast<int>(myPos.z / GRID_CELL_SIZE);
        int searchRadius = static_cast<int>(20.0f / GRID_CELL_SIZE) + 1;
        
        for (int dx = -searchRadius; dx <= searchRadius; dx++) {
            for (int dz = -searchRadius; dz <= searchRadius; dz++) {
                int key = ((centerX + dx) << 16) | ((centerZ + dz) & 0xFFFF);
                
                if (enemyGrid.find(key) == enemyGrid.end()) continue;
                
                for (Entity targetEntity : enemyGrid[key]) {
                    if (!gCoordinator.HasComponent<TransformComponent>(targetEntity)) continue;
                    
                    auto& tPos = gCoordinator.GetComponent<TransformComponent>(targetEntity).Pos;
                    float distSq = (tPos.x - myPos.x) * (tPos.x - myPos.x) + 
                                   (tPos.z - myPos.z) * (tPos.z - myPos.z);

                    if (distSq < minDstSq) {
                        minDstSq = distSq;
                        nearest = targetEntity;
                    }
                }
            }
        }
        
        return nearest;
    }
};