#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h" 
#include "FactionComponent.h"
#include "ProjectileComponent.h"
#include <vector>
#include <set>

extern Coordinator gCoordinator;

class CollisionSystem : public System
{
public:
    void Update(float dt)
    {
        // Copy entities to vector for indexed O(N^2) access
        auto entities = std::vector<Entity>(mEntities.begin(), mEntities.end());
        std::set<Entity> destroyedThisFrame;

        for (size_t i = 0; i < entities.size(); ++i)
        {
            Entity entityA = entities[i];
            if (destroyedThisFrame.count(entityA)) continue;

            for (size_t j = i + 1; j < entities.size(); ++j)
            {
                Entity entityB = entities[j];
                if (destroyedThisFrame.count(entityB)) continue;

                // 1. Physical Collision Check
                if (CheckCollision(entityA, entityB))
                {
                    ResolveCollision(entityA, entityB, destroyedThisFrame);

                    // If A died in this interaction, stop checking A against others
                    if (destroyedThisFrame.count(entityA)) break;
                }
            }
        }
    }

private:
    bool CheckCollision(Entity a, Entity b)
    {
        auto& transA = gCoordinator.GetComponent<TransformComponent>(a);
        auto& colA   = gCoordinator.GetComponent<ColliderComponent>(a);
        auto& transB = gCoordinator.GetComponent<TransformComponent>(b);
        auto& colB   = gCoordinator.GetComponent<ColliderComponent>(b);

        float dx = transA.Pos.x - transB.Pos.x;
        float dy = transA.Pos.y - transB.Pos.y;
        float dz = transA.Pos.z - transB.Pos.z;

        float distSq = dx * dx + dy * dy + dz * dz;
        float radiiSum = colA.radius + colB.radius;

        return distSq < (radiiSum * radiiSum);
    }

    void ResolveCollision(Entity a, Entity b, std::set<Entity>& destroyedSet)
    {
        // 1. Ensure both have Stats (Health/Damage/Team)
        if (!gCoordinator.HasComponent<StatComponent>(a) || 
            !gCoordinator.HasComponent<StatComponent>(b)) return;

        auto& statA = gCoordinator.GetComponent<StatComponent>(a);
        auto& statB = gCoordinator.GetComponent<StatComponent>(b);

        // 2. Friendly Fire Check
        if (statA.teamID == statB.teamID) return;

        // 3. Apply Damage (Mutual Exchange)
        // If 'damage' is 0 (like a Ranged Unit touching someone), nothing happens.
        ApplyDamage(a, statA, statB.damage, destroyedSet);
        if(destroyedSet.count(a)) {
             // If A died, we still apply A's damage to B (e.g., bullet hits B)
             // But we don't access 'a' components anymore if we destroyed it inside ApplyDamage
        }
        ApplyDamage(b, statB, statA.damage, destroyedSet);

        // 4. Handle Projectile Self-Destruction
        // Even if the projectile didn't die from "Health", it dies because it hit something.
        HandleProjectileBehavior(a, destroyedSet);
        HandleProjectileBehavior(b, destroyedSet);
    }

    void ApplyDamage(Entity e, StatComponent& stats, int damageAmount, std::set<Entity>& destroyedSet)
    {
        if (destroyedSet.count(e)) return; // Already dead

        stats.health -= damageAmount;

        if (stats.health <= 0)
        {
            DestroyEntity(e, destroyedSet);
        }
    }

    void HandleProjectileBehavior(Entity e, std::set<Entity>& destroyedSet)
    {
        if (destroyedSet.count(e)) return;

        // If it is a projectile, it dies on ANY valid collision
        if (gCoordinator.HasComponent<ProjectileComponent>(e))
        {
            DestroyEntity(e, destroyedSet);
        }
    }

    void DestroyEntity(Entity e, std::set<Entity>& destroyedSet)
    {
        gCoordinator.DestroyEntity(e);
        destroyedSet.insert(e);
    }
};