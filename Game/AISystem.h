#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h" 
#include "UnitSystem.h" // Needed to see all units
#include "MeshBuilder.h"
#include "ProjectileComponent.h"

extern Coordinator gCoordinator;
extern std::shared_ptr<UnitSystem> unitSystem; // Access global unit list

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
            auto& unit = gCoordinator.GetComponent<UnitComponent>(entity); // GET UNIT COMPONENT

            // 3. MOVEMENT LOGIC (Wander)
            if (ai.type == AIComponent::Type::Wander)
            {
                // If we aren't moving, or timer expired, pick a new spot
                if (!unit.isMoving || ai.actionTimer <= 0)
                {
                    // Pick new random spot
                    float rX = ((rand() % 100) / 10.0f) - 5.0f; // -5 to 5
                    float rZ = ((rand() % 100) / 10.0f) - 5.0f;
                    
                    // --- THE FIX: DON'T MOVE TRANSFORM. SET TARGET. ---
                    unit.targetPos = { transform.Pos.x + rX, 0, transform.Pos.z + rZ };
                    unit.isMoving = true;
                    
                    ai.actionTimer = 4000.0f; // Wander for 4 seconds
                }
            }
        }
    }

private:
    Entity FindNearestEnemy(vec3d myPos, int myTeam)
    {
        Entity nearest = -1;
        float minDstSq = 15.0f * 15.0f; // Aggro Range Squared

        // Search through ALL units (handled by UnitSystem)
        for (auto const& targetEntity : unitSystem->mEntities)
        {
            // Skip self
            if (!gCoordinator.HasComponent<FactionComponent>(targetEntity)) continue;

            auto& targetFaction = gCoordinator.GetComponent<FactionComponent>(targetEntity);
            
            // Only target enemies
            if (targetFaction.teamId != myTeam)
            {
                auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(targetEntity);
                
                float dx = targetTrans.Pos.x - myPos.x;
                float dz = targetTrans.Pos.z - myPos.z;
                float distSq = dx*dx + dz*dz;

                if (distSq < minDstSq)
                {
                    minDstSq = distSq;
                    nearest = targetEntity;
                }
            }
        }
        return nearest;
    }

    void SpawnProjectile(vec3d startPos, Entity targetID, int ownerTeam)
    {
        // ... (Keep your existing SpawnProjectile code) ...
        // Ensure you create the entity and add ProjectileComponent correctly
         auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(targetID);
        vec3d dir = Engine3D::Vector_Sub(targetTrans.Pos, startPos);
        dir = Engine3D::Vector_Normalise(dir);
        vec3d velocity = Engine3D::Vector_Mul(dir, 15.0f); 

        Entity bullet = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(bullet, TransformComponent{ startPos });
        gCoordinator.AddComponent(bullet, MeshComponent{ ShapeBuilder::CreateCube(0.2f, 1, 0, 0) });
        gCoordinator.AddComponent(bullet, ColliderComponent{ 0.5f, true, false });
        gCoordinator.AddComponent(bullet, ProjectileComponent{ velocity, 10, ownerTeam, 2.0f });
    }
};;