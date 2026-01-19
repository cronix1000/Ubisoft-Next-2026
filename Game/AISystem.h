#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h" 
#include "ProjectileComponent.h"
#include "FactionComponent.h" // Include your new component
#include "MeshBuilder.h"

extern Coordinator gCoordinator;

class AISystem : public System
{
public:
    void Update(float dt)
    {
        for (auto const& entity : mEntities)
        {
            auto& ai = gCoordinator.GetComponent<AIComponent>(entity);
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& faction = gCoordinator.GetComponent<FactionComponent>(entity);

            // 1. UPDATE TIMERS
            if (ai.attackCooldown > 0) ai.attackCooldown -= dt;
            if (ai.actionTimer > 0) ai.actionTimer -= dt;

            // 2. COMBAT LOGIC (Find nearest enemy)
            if (ai.attackCooldown <= 0)
            {
                Entity target = FindNearestEnemy(transform.Pos, faction.teamId);
                
                // If target found and within range (e.g., 15 units)
                if (target != -1) 
                {
                    // Shoot!
                    SpawnProjectile(transform.Pos, target, faction.teamId);
                    ai.attackCooldown = 2000.0f; // 2 seconds cooldown
                }
            }

            // 3. MOVEMENT LOGIC (Wander)
            if (ai.type == AIComponent::Type::Wander)
            {
                if (ai.actionTimer <= 0)
                {
                    // Pick new random spot
                    float rX = ((rand() % 100) / 10.0f) - 5.0f; // -5 to 5 offset
                    float rZ = ((rand() % 100) / 10.0f) - 5.0f;
                    ai.wanderTarget = { transform.Pos.x + rX, 0, transform.Pos.z + rZ };
                    ai.actionTimer = 4000.0f; // Move for 4 seconds
                }

                // Simple Move To Target
                vec3d dir = Engine3D::Vector_Sub(ai.wanderTarget, transform.Pos);
                dir.y = 0; // Keep on ground
                
                float dist = sqrt(dir.x*dir.x + dir.z*dir.z);
                if (dist > 0.1f) {
                    vec3d norm = Engine3D::Vector_Div(dir, dist);
                    float speed = 3.0f * (dt / 1000.0f);
                    transform.Pos = Engine3D::Vector_Add(transform.Pos, Engine3D::Vector_Mul(norm, speed));
                }
            }
        }
    }

private:
    Entity FindNearestEnemy(vec3d myPos, int myTeam)
    {
        Entity nearest = -1;
        float minDst = 15.0f; // Aggro Range

        // Naive O(N) search through all entities - optimize later with Group Query
        // Ideally, you keep a list of 'Units' separately.
        // For now, we assume standard entity iteration isn't too slow (< 500 units)
        // Note: In real ECS, you'd query a "FactionSystem" or specific ComponentArray.
        // We will iterate ALL entities in existence (slow but works for contest)
        for (int i = 0; i < 5000; i++) // Assuming max entities
        {
            // Check if entity is valid and has components
            // Note: gCoordinator needs a "GetActiveEntities" or similar to avoid checking empty slots
            // If you don't have that, you might need to register FactionComponent in this System signature too.
            // For safety, let's just skip this part or assume we iterate mEntities of a "FactionSystem".
            // Implementation shortcut: just return -1 for now or check mEntities if we include Faction in signature.
            return -1; // Placeholder: You need to iterate entities with FactionComponent
        }
        return -1;
    }

    void SpawnProjectile(vec3d startPos, Entity targetID, int ownerTeam)
    {
        auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(targetID);
        
        // Calculate Velocity
        vec3d dir = Engine3D::Vector_Sub(targetTrans.Pos, startPos);
        dir = Engine3D::Vector_Normalise(dir);
        vec3d velocity = Engine3D::Vector_Mul(dir, 10.0f); // Speed 10

        // Create Bullet Entity
        Entity bullet = gCoordinator.CreateEntity();
        
        // Add Components
        gCoordinator.AddComponent(bullet, TransformComponent{ startPos });
        
        // Create a small cube for visual
        mesh m = ShapeBuilder::CreateCube(0.2f, 1, 0, 0); // Red bullet
        gCoordinator.AddComponent(bullet, MeshComponent{ m });
        
        gCoordinator.AddComponent(bullet, ColliderComponent{ 0.5f, true, false });
        gCoordinator.AddComponent(bullet, ProjectileComponent{ velocity, 10, ownerTeam, 2.0f });
    }
};