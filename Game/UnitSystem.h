#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h" 
#include "ProjectileComponent.h"
#include "MeshBuilder.h"
#include "ArcAnimComponent.h"

extern Coordinator gCoordinator;

class UnitSystem : public System
{
public:
    void Update(float dt)
    {
        static float timeTracker = 0.0f;
        timeTracker += dt / 1000.0f;

        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<UnitComponent>(entity)) continue;

            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& unit = gCoordinator.GetComponent<UnitComponent>(entity);

            // Assume units have AIComponent for stats/cooldowns
            // and FactionComponent for team ID
            // If not, add them to your entity creation!
            bool hasCombat = gCoordinator.HasComponent<AIComponent>(entity) &&
                gCoordinator.HasComponent<FactionComponent>(entity);

            // --- COMBAT LOGIC ---
            if (hasCombat)
            {
                auto& ai = gCoordinator.GetComponent<AIComponent>(entity);
                auto& faction = gCoordinator.GetComponent<FactionComponent>(entity);

                // Tick Cooldown
                if (ai.attackCooldown > 0) ai.attackCooldown -= dt;

                // Check Range
                Entity target = FindNearestEnemy(transform.Pos, faction.teamId);
                if (target != -1)
                {
                    auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(target);
                    float dist = Engine3D::Vector_Distance(transform.Pos, targetTrans.Pos);

                    // If in range, STOP and FIRE
                    if (dist <= ai.attackRange)
                    {
                        //unit.isMoving = false; // Override Squad command

                        if (ai.attackCooldown <= 0)
                        {
                            SpawnProjectile(transform.Pos, targetTrans.Pos,target, faction.teamId);
                            ai.attackCooldown = 2000.0f; // 2 Seconds
                        }
                    }
                }
            }

            // --- MOVEMENT LOGIC ---
            if (unit.isMoving)
            {
                vec3d diff = Engine3D::Vector_Sub(unit.targetPos, transform.Pos);
                diff.y = 0.0f;
                float dist = sqrtf(diff.x * diff.x + diff.z * diff.z);
                float moveStep = unit.speed * (dt / 1000.0f);

                if (dist <= moveStep || dist < 0.05f)
                {
                    transform.Pos.x = unit.targetPos.x;
                    transform.Pos.z = unit.targetPos.z;
                }
                else
                {
                    vec3d dir = { diff.x / dist, 0.0f, diff.z / dist };
                    transform.Pos.x += dir.x * moveStep;
                    transform.Pos.z += dir.z * moveStep;
                }
            }
            // --- IDLE ANIMATION ---
            else
            {
                float seed = (float)entity * 1.3f;
                float swayX = sinf(timeTracker * 2.0f + seed) * 0.1f;
                float swayZ = cosf(timeTracker * 1.7f + seed * 3.0f) * 0.1f;
                // Sway around current position
                transform.Pos.x += swayX * 0.1f;
                transform.Pos.z += swayZ * 0.1f;
            }
        }
    }

private:
    Entity FindNearestEnemy(vec3d myPos, int myTeam)
    {
        Entity nearest = -1;
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

    void SpawnProjectile(vec3d startPos, vec3d endPos, Entity targetID, int ownerTeam)
    {
        auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(targetID);
        vec3d dir = Engine3D::Vector_Normalise(Engine3D::Vector_Sub(targetTrans.Pos, startPos));
        vec3d velocity = Engine3D::Vector_Mul(dir, 15.0f);

        Entity bullet = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(bullet, TransformComponent{ startPos });
        gCoordinator.AddComponent(bullet, MeshComponent{ ShapeBuilder::CreateCubeScales({0.5, 0.5, 0.5},0.2f, 1, 0, 0) });
        gCoordinator.AddComponent(bullet, ColliderComponent{ 0.5f, true, false });
        gCoordinator.AddComponent(bullet, ProjectileComponent{ velocity, 10, ownerTeam, 2.0f });
        gCoordinator.AddComponent(bullet, ArcAnimComponent{ startPos, endPos, 2.0f, 0.8f, 0.0f });
    }
};