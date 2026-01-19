#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h" 
#include "FactionComponent.h"
#include "ProjectileComponent.h"

extern Coordinator gCoordinator;

class CollisionSystem : public System
{
public:
    void Update(float dt)
    {
        // 1. Separate Projectiles from Targets
        // In a robust engine, you'd use a QuadTree. For < 500 units, iterating is fine.
        std::vector<Entity> projectiles;
        std::vector<Entity> targets;

        for (auto const& entity : mEntities)
        {
            if (gCoordinator.HasComponent<ProjectileComponent>(entity)) {
                projectiles.push_back(entity);
            }
            else {
                targets.push_back(entity);
            }
        }

        // 2. Check Collisions (Projectile vs Target)
        for (Entity bullet : projectiles)   
        {
            auto& bulletTrans = gCoordinator.GetComponent<TransformComponent>(bullet);
            auto& bulletCol = gCoordinator.GetComponent<ColliderComponent>(bullet);
            auto& bulletProj = gCoordinator.GetComponent<ProjectileComponent>(bullet);

            for (Entity target : targets)
            {
                // Safety Check: Don't hit yourself or your own team
                if (gCoordinator.HasComponent<FactionComponent>(target)) {
                    auto& faction = gCoordinator.GetComponent<FactionComponent>(target);
                    if (faction.teamId == bulletProj.ownerTeamId) continue;
                }

                auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(target);
                auto& targetCol = gCoordinator.GetComponent<ColliderComponent>(target);

                // --- SPHERE COLLISION MATH ---
                // Formula: Distance^2 < (Radius1 + Radius2)^2
                float dx = bulletTrans.Pos.x - targetTrans.Pos.x;
                float dy = bulletTrans.Pos.y - targetTrans.Pos.y;
                float dz = bulletTrans.Pos.z - targetTrans.Pos.z;

                float distSq = dx * dx + dy * dy + dz * dz;
                float radiiSum = bulletCol.radius + targetCol.radius;

                if (distSq < (radiiSum * radiiSum))
                {
                    // HIT!
                    HandleCollision(bullet, target);
                    break; // Bullet is gone, stop checking this bullet
                }
            }
        }
    }

private:
    void HandleCollision(Entity bullet, Entity target)
    {
        // 1. Apply Damage
        //if (gCoordinator.HasComponent<CombatStats>(target)) {
        //    auto& stats = gCoordinator.GetComponent<CombatStats>(target);
        //    auto& proj = gCoordinator.GetComponent<Projectile>(bullet);

        //    stats.health -= proj.damage;

        //    // Death Logic
        //    if (stats.health <= 0) {
        //        gCoordinator.DestroyEntity(target);
        //        // TODO: Spawn a particle effect or drop gold here
        //    }
        //}

        // 2. Destroy Bullet
        gCoordinator.DestroyEntity(bullet);
    }
};