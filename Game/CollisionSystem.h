#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h" 
#include "FactionComponent.h"
#include "ProjectileComponent.h"
#include "UnitComponent.h" // Added to access cooldowns
#include <vector>
#include <set>
#include <algorithm>

extern Coordinator gCoordinator;

// Helper struct to cache pointers
struct ColliderWrapper {
    Entity entity;
    TransformComponent* transform;
    ColliderComponent* collider;
    StatComponent* stats;
    UnitComponent* unit;       
    ProjectileComponent* proj; 
};

class CollisionSystem : public System
{
public:
    void Update(float dt)
    {
        std::vector<ColliderWrapper> colliders;
        colliders.reserve(mEntities.size());

        // 1. CACHE STEP
        for (auto const& entity : mEntities)
        {
            // Basic Requirements
            if (!gCoordinator.HasComponent<TransformComponent>(entity) ||
                !gCoordinator.HasComponent<ColliderComponent>(entity) ||
                !gCoordinator.HasComponent<StatComponent>(entity))
            {
                continue;
            }

            ColliderWrapper cw;
            cw.entity = entity;
            cw.transform = &gCoordinator.GetComponent<TransformComponent>(entity);
            cw.collider = &gCoordinator.GetComponent<ColliderComponent>(entity);
            cw.stats = &gCoordinator.GetComponent<StatComponent>(entity);
           

            // Unit Component (For Cooldowns)
            if (gCoordinator.HasComponent<UnitComponent>(entity)) {
                cw.unit = &gCoordinator.GetComponent<UnitComponent>(entity);
                // UPDATE TIMER HERE:
                if (cw.unit->actionTimer > 0.0f) {
                    cw.unit->actionTimer -= dt;
                }
            } else {
                cw.unit = nullptr;
            }

            // Projectile Component (For one-shot logic)
            if (gCoordinator.HasComponent<ProjectileComponent>(entity)) {
                cw.proj = &gCoordinator.GetComponent<ProjectileComponent>(entity);
            } else {
                cw.proj = nullptr;
            }

            colliders.push_back(cw);
        }

        // 2. SORT STEP (Sweep and Prune X-Axis)
        std::sort(colliders.begin(), colliders.end(), 
            [](const ColliderWrapper& a, const ColliderWrapper& b) {
                return a.transform->Pos.x < b.transform->Pos.x;
            });

        std::set<Entity> destroyedThisFrame;

        // 3. COLLISION LOOP
        for (size_t i = 0; i < colliders.size(); ++i)
        {
            if (destroyedThisFrame.count(colliders[i].entity)) continue;

            for (size_t j = i + 1; j < colliders.size(); ++j)
            {
                if (destroyedThisFrame.count(colliders[j].entity)) continue;

                // X-Axis Early Exit
                float xDiff = colliders[j].transform->Pos.x - colliders[i].transform->Pos.x;
                float radiusSum = colliders[i].collider->radius + colliders[j].collider->radius;
                
                if (xDiff > radiusSum) break; 

                if (CheckCollision(colliders[i], colliders[j], radiusSum))
                {
                    ResolveCollision(colliders[i], colliders[j], destroyedThisFrame);
                    if (destroyedThisFrame.count(colliders[i].entity)) break;
                }
            }
        }
    }

private:
    bool CheckCollision(const ColliderWrapper& a, const ColliderWrapper& b, float radiusSum)
    {
        float dy = a.transform->Pos.y - b.transform->Pos.y;
        float dz = a.transform->Pos.z - b.transform->Pos.z;
        float dx = a.transform->Pos.x - b.transform->Pos.x; // Recalculate full delta

        if (abs(dz) > radiusSum) return false; // Quick Z Check

        float distSq = dx * dx + dy * dy + dz * dz;
        return distSq < (radiusSum * radiusSum);
    }

    void ResolveCollision(ColliderWrapper& a, ColliderWrapper& b, std::set<Entity>& destroyedSet)
    {
        // 1. Team Check (Friendly Fire prevention)
        if (a.stats && b.stats) {
            if (a.stats->teamID== b.stats->teamID) return;
        }

        // 2. Projectile Logic (One-shot)
        // If A is a projectile, it hits B, deals damage, and dies.
        if (a.proj) {
            ApplyDamage(b.entity, *b.stats, a.stats->damage, destroyedSet);
            DestroyEntity(a.entity, destroyedSet);
            return; // A is dead, stop interaction
        }
        // If B is a projectile, it hits A, deals damage, and dies.
        if (b.proj) {
            ApplyDamage(a.entity, *a.stats, b.stats->damage, destroyedSet);
            DestroyEntity(b.entity, destroyedSet);
            return; // B is dead, stop interaction
        }

        // 3. Unit Combat Logic (Melee / Contact)
        
        // B attacks A?
        bool bCanAttack = true;
        if (b.unit) {
            // If it's a unit, it must wait for cooldown
            if (b.unit->actionTimer > 0.0f) bCanAttack = false;
        }
        
        if (bCanAttack) {
            ApplyDamage(a.entity, *a.stats, b.stats->damage, destroyedSet);
            // Reset B's Cooldown
            if (b.unit) {
                // Safety: If attackCooldown is 0 (default), set a minimum (0.5s) to prevent insta-kill
                float cooldown = (b.unit->attackCooldown > 0.0f) ? b.unit->attackCooldown : 1.0f;
                b.unit->actionTimer = cooldown;
            }
        }

        // A attacks B? (Only if A is still alive)
        if (destroyedSet.count(a.entity)) return;

        bool aCanAttack = true;
        if (a.unit) {
            if (a.unit->actionTimer > 0.0f) aCanAttack = false;
        }

        if (aCanAttack) {
            ApplyDamage(b.entity, *b.stats, a.stats->damage, destroyedSet);
            // Reset A's Cooldown
            if (a.unit) {
                float cooldown = (a.unit->attackCooldown > 0.0f) ? a.unit->attackCooldown : 1.0f;
                a.unit->actionTimer = cooldown;
            }
        }
    }

    void ApplyDamage(Entity e, StatComponent& stats, int damageAmount, std::set<Entity>& destroyedSet)
    {
        if (destroyedSet.count(e)) return;

        stats.health -= damageAmount;
        if (stats.health <= 0)
        {
            DestroyEntity(e, destroyedSet);
        }
    }

    void DestroyEntity(Entity e, std::set<Entity>& destroyedSet)
    {
        if (destroyedSet.count(e)) return;
        if (gCoordinator.HasComponent<OccupyingDepositComponent>(e)) {
                auto& link = gCoordinator.GetComponent<OccupyingDepositComponent>(e);
                
                // Check if the gold chunk still exists (it should, but safety first)
                // Note: We use a simple check or try/catch pattern if available, 
                // but in this ECS, assuming the entity ID is valid is standard.
                if (gCoordinator.HasComponent<GoldDepositComponent>(link.goldChunkEntity)) {
                    auto& deposit = gCoordinator.GetComponent<GoldDepositComponent>(link.goldChunkEntity);
                    deposit.occupied = false; 
                    
                    // Optional: Visual Feedback (Tint it back to Gold to show it's active)
                    if (gCoordinator.HasComponent<MeshComponent>(link.goldChunkEntity)) {
                        auto& meshComp = gCoordinator.GetComponent<MeshComponent>(link.goldChunkEntity);
                        // Reset to Gold Color (R=1.0, G=0.8, B=0.0)
                        for (auto& tri : meshComp.mesh.tris) {
                            tri.r = 1.0f; tri.g = 0.8f; tri.b = 0.0f;
                        }
                    }
                }
            }

        gCoordinator.DestroyEntity(e);
        destroyedSet.insert(e);
    }
};