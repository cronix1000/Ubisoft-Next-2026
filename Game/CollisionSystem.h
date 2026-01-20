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

const float CELL_SIZE = 2.0f; 

struct GridKey {
    int x, z;
    bool operator==(const GridKey& other) const { return x == other.x && z == other.z; }
};

// Hasher for the map
struct KeyHasher {
    std::size_t operator()(const GridKey& k) const {
        return std::hash<int>()(k.x) ^ std::hash<int>()(k.z);
    }
};

class CollisionSystem : public System {
    std::unordered_map<GridKey, std::vector<Entity>, KeyHasher> grid;
    private:
    std::vector<ColliderWrapper> colliders;

public:
    void Init() {
        colliders.reserve(2000); // Pre-allocate for max expected units
    }
    void Update(float dt)
    {
        colliders.clear();
        colliders.reserve(mEntities.size());

        // 1. CACHE STEP - also cache team IDs to avoid lookups later
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

        // Early exit if too few entities
        if (colliders.size() < 2) return;
        
        // 2. SORT STEP (Sweep and Prune X-Axis)
        std::sort(colliders.begin(), colliders.end(), 
            [](const ColliderWrapper& a, const ColliderWrapper& b) {
                return a.transform->Pos.x < b.transform->Pos.x;
            });

        std::set<Entity> destroyedThisFrame;

        // 3. COLLISION LOOP - with better early exits
        for (size_t i = 0; i < colliders.size(); ++i)
        {
            if (destroyedThisFrame.count(colliders[i].entity)) continue;
            
            float iX = colliders[i].transform->Pos.x;
            float iRadius = colliders[i].collider->radius;

            for (size_t j = i + 1; j < colliders.size(); ++j)
            {
                if (destroyedThisFrame.count(colliders[j].entity)) continue;

                // X-Axis Early Exit
                float xDiff = colliders[j].transform->Pos.x - iX;
                float radiusSum = iRadius + colliders[j].collider->radius;
                
                if (xDiff > radiusSum) break; 
                
                // Early team check - skip if same team
                if (colliders[i].stats->teamID == colliders[j].stats->teamID) continue;

                if (CheckCollision(colliders[i], colliders[j], radiusSum))
                {
                    ResolveCollision(colliders[i], colliders[j], destroyedThisFrame);
                    if (destroyedThisFrame.count(colliders[i].entity)) break;
                }
            }
        }
    }

private:
void SpawnExplosion(vec3d center, float r, float g, float b)
{
    for (int i = 0; i < 20; i++) 
    {
        Entity p = gCoordinator.CreateEntity();

        // Jitter (Position offset)
        float jitterX = (rand() % 10 - 5) / 20.0f; 
        float jitterY = (rand() % 10 - 5) / 20.0f; 
        float jitterZ = (rand() % 10 - 5) / 20.0f; 
        
        vec3d spawnPos = { center.x + jitterX, center.y + jitterY, center.z + jitterZ };
        gCoordinator.AddComponent(p, TransformComponent{ spawnPos });
        
        // --- REDUCED VELOCITY ---
        // X and Z: Range is now -3.0 to +3.0 (was -10 to +10)
        float vx = (rand() % 60 - 30) / 10.0f; 
        
        // Y (Upward): Range is now +1.0 to +6.0 (was +2 to +22)
        float vy = (rand() % 50) / 10.0f + 1.0f; 
        
        // Z: Range is now -3.0 to +3.0
        float vz = (rand() % 60 - 30) / 10.0f;
        // ------------------------

        float scaleStart = (rand() % 10 + 5) / 100.0f; // Tiny size (0.05 to 0.15)

        gCoordinator.AddComponent(p, ParticleComponent{ 
            {vx, vy, vz}, 
            0.8f, 1.0f,      // Lifetime
            scaleStart, 0.0f,// Shrink
            r, g, b 
        });

        gCoordinator.AddComponent(p, MeshComponent{ 
            ShapeBuilder::CreateCubeScales({scaleStart, scaleStart, scaleStart}, 1.0f, r, g, b) 
        });
    }
}
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
        // 1. Projectile Logic (One-shot or Splash)
        // If A is a projectile, it hits B, deals damage, and dies.
        if (a.proj) {
            // Check for splash damage (AoE)
            if (a.proj->splashRadius > 0.0f) {
                ApplySplashDamage(*a.transform, a.stats->damage, a.proj->ownerTeamId, a.proj->splashRadius, destroyedSet);
            } else {
                // Single target damage
                ApplyDamage(b.entity, *b.stats, a.stats->damage, destroyedSet);
            }
            DestroyEntity(a.entity, destroyedSet);
            return; // A is dead, stop interaction
        }
        // If B is a projectile, it hits A, deals damage, and dies.
        if (b.proj) {
            // Check for splash damage (AoE)
            if (b.proj->splashRadius > 0.0f) {
                ApplySplashDamage(*b.transform, b.stats->damage, b.proj->ownerTeamId, b.proj->splashRadius, destroyedSet);
            } else {
                // Single target damage
                ApplyDamage(a.entity, *a.stats, b.stats->damage, destroyedSet);
            }
            DestroyEntity(b.entity, destroyedSet);
            return; // B is dead, stop interaction
        }

        // 2. Unit Combat Logic (Melee / Contact)
        
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

        // if (gCoordinator.HasComponent<MeshComponent>(e)) {
        //     auto& meshComp = gCoordinator.GetComponent<MeshComponent>(e);
        //     ShapeBuilder::TintMesh(meshComp.mesh, 1.0f, 0.0f, 0.0f); 
        // }

        // if (gCoordinator.HasComponent<UnitComponent>(e)) {
        //     auto& unit = gCoordinator.GetComponent<UnitComponent>(e);
        //     unit.flashTimer = 0.1f; 
        // }

    
        stats.health -= damageAmount;
        if (stats.health <= 0)
        {
            DestroyEntity(e, destroyedSet);
        }
    }

    void ApplySplashDamage(TransformComponent& impactPos, int damage, int ownerTeam, float radius, std::set<Entity>& destroyedSet)
    {
        // Deal damage to all enemies within splash radius
        for (auto const& entity : mEntities)
        {
            if (destroyedSet.count(entity)) continue;
            if (!gCoordinator.HasComponent<StatComponent>(entity)) continue;
            if (!gCoordinator.HasComponent<TransformComponent>(entity)) continue;
            if (!gCoordinator.HasComponent<FactionComponent>(entity)) continue;
            auto& stats = gCoordinator.GetComponent<StatComponent>(entity);
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& faction = gCoordinator.GetComponent<FactionComponent>(entity);

            // Don't damage same team
            if (faction.teamId == ownerTeam) continue;

            // Check if within splash radius
            float dist = Engine3D::Vector_Distance(impactPos.Pos, trans.Pos);
            if (dist <= radius) {
                // Apply full damage at center, 50% at edge (linear falloff)
                float damageMultiplier = 1.0f - (dist / radius) * 0.5f;
                int actualDamage = (int)(damage * damageMultiplier);
                ApplyDamage(entity, stats, actualDamage, destroyedSet);
            }
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
                    deposit.linkedFactory = static_cast<Entity>(-1);
                    
                    // Optional: Visual Feedback (Tint it back to Gold to show it's active)
                    if (gCoordinator.HasComponent<MeshComponent>(link.goldChunkEntity)) {
                        auto& meshComp = gCoordinator.GetComponent<MeshComponent>(link.goldChunkEntity);
                        // Reset to Gold Color (R=1.0, G=0.8, B=0.0)
                        for (auto& tri : meshComp.mesh.tris) {
                            tri.r = 1.0f; tri.g = 0.8f; tri.b = 0.0f;
                        }
                    }
                }

                App::PlayAudio("./data/explosion.mp3",false);
                // Spawn Explosion Particles at Entity Position
                if (gCoordinator.HasComponent<TransformComponent>(e)) {
                auto& trans = gCoordinator.GetComponent<TransformComponent>(e);
                SpawnExplosion(trans.Pos, 1.0f, 0.5f, 0.0f);
            }
    
        }
        //if(gCoordinator.HasComponent<UnitComponent>(e)) {
        //    auto& trans = gCoordinator.GetComponent<TransformComponent>(e);
        //    SpawnExplosion(trans.Pos, 1.0f, 0.5f, 0.0f);
        //}
        gCoordinator.DestroyEntity(e);
        destroyedSet.insert(e);
    
    
    }
};

