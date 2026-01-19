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
        // 1. Copy entities to a vector for indexed access
        // (This allows us to perform the O(N^2) check: Entity A vs Entity B)
        std::vector<Entity> entities(mEntities.begin(), mEntities.end());
        std::set<Entity> destroyedThisFrame;

        // 2. Iterate Unique Pairs
        for (size_t i = 0; i < entities.size(); ++i)
        {
            Entity entityA = entities[i];

            // Skip if A was destroyed earlier in the frame
            if (destroyedThisFrame.count(entityA)) continue;

            for (size_t j = i + 1; j < entities.size(); ++j)
            {
                Entity entityB = entities[j];

                // Skip if B was destroyed earlier
                if (destroyedThisFrame.count(entityB)) continue;

                // 3. Check Collision
                if (CheckCollision(entityA, entityB))
                {
                    ResolveCollision(entityA, entityB, destroyedThisFrame);

                    // --- THE FIX ---
                    // If Entity A was destroyed during this collision (e.g. it was the bullet or the victim), 
                    // we MUST stop comparing it to others immediately.
                    if (destroyedThisFrame.count(entityA))
                    {
                        break;
                    }
                }
            }
        }
    }

private:
    bool CheckCollision(Entity a, Entity b)
    {
        // Get Components (Assumes entities exist due to destroyedThisFrame check)
        auto& transA = gCoordinator.GetComponent<TransformComponent>(a);
        auto& colA = gCoordinator.GetComponent<ColliderComponent>(a);

        auto& transB = gCoordinator.GetComponent<TransformComponent>(b);
        auto& colB = gCoordinator.GetComponent<ColliderComponent>(b);

        float dx = transA.Pos.x - transB.Pos.x;
        float dy = transA.Pos.y - transB.Pos.y;
        float dz = transA.Pos.z - transB.Pos.z;

        float distSq = dx * dx + dy * dy + dz * dz;
        float radiiSum = colA.radius + colB.radius;

        return distSq < (radiiSum * radiiSum);
    }

    void ResolveCollision(Entity a, Entity b, std::set<Entity>& destroyedSet)
    {
        bool aIsProj = gCoordinator.HasComponent<ProjectileComponent>(a);
        bool bIsProj = gCoordinator.HasComponent<ProjectileComponent>(b);

        // --- CASE 1: Projectile vs Unit ---
        if (aIsProj && !bIsProj) {
            HandleProjectileHit(a, b, destroyedSet);
        }
        else if (bIsProj && !aIsProj) {
            HandleProjectileHit(b, a, destroyedSet);
        }
        // --- CASE 2: Unit vs Unit (Optional Physics) ---
        else if (!aIsProj && !bIsProj) {
            // Logic for units bumping into each other (Pushback) goes here
            // e.g. PushBack(a, b);
        }
    }

    void HandleProjectileHit(Entity projectile, Entity target, std::set<Entity>& destroyedSet)
    {
        auto& proj = gCoordinator.GetComponent<ProjectileComponent>(projectile);

        // Friendly Fire Check
        if (gCoordinator.HasComponent<FactionComponent>(target)) {
            auto& faction = gCoordinator.GetComponent<FactionComponent>(target);
            // If same team, ignore collision entirely
            if (faction.teamId == proj.ownerTeamId) return;
        }

        // --- APPLY DAMAGE LOGIC ---
        //// if (gCoordinator.HasComponent<CombatStats>(target)) { ... }
        gCoordinator.DestroyEntity(target);
        destroyedSet.insert(target);

        // --- DESTROY PROJECTILE ---
        gCoordinator.DestroyEntity(projectile);
        destroyedSet.insert(projectile); // Mark as dead so we don't process it again this loop
    }
};