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
                auto& faction = gCoordinator.GetComponent<FactionComponent>(entity);

                if (unit.attackCooldown > 0) unit.attackCooldown -= dt;
                
                if (unit.targetEntity != -1)
                {
                    if (!gCoordinator.HasComponent<TransformComponent>(unit.targetEntity)) continue;
                    auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(unit.targetEntity);
                    float dist = Engine3D::Vector_Distance(transform.Pos, targetTrans.Pos);
                  
                    if (dist <= unit.attackRange)
                    {
                        if (unit.attackCooldown <= 0)
                        { 
                             switch(unit.type){
                        case UnitComponent::UnitType::meleeGrunt:
                            unit.attackRange = 2.0f;
                            break;
                        case UnitComponent::UnitType::Ranged:
                            SpawnBullet(transform.Pos, unit.targetEntity, faction.teamId);
                            break;
                        case UnitComponent::UnitType::Catapult:
                            SpawnCatapultRock(transform.Pos, unit.targetEntity, faction.teamId);
                            unit.attackCooldown = 4000.0f;
                            break;
                    }
                            unit.attackCooldown = 2000.0f;
                        }
                    }
                }
            
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
            else
            {
                float seed = (float)entity * 1.3f;
                float swayX = sinf(timeTracker * 2.0f + seed) * 0.1f;
                float swayZ = cosf(timeTracker * 1.7f + seed * 3.0f) * 0.1f;
                transform.Pos.x += swayX * 0.1f;
                transform.Pos.z += swayZ * 0.1f;
            }
        }
    }

private:
   

    void SpawnBullet(vec3d startPos, Entity targetID, int ownerTeam)
    {
        if (!gCoordinator.HasComponent<TransformComponent>(targetID)) return;
        auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(targetID);
        vec3d dir = Engine3D::Vector_Normalise(Engine3D::Vector_Sub(targetTrans.Pos, startPos));
        vec3d velocity = Engine3D::Vector_Mul(dir, 15.0f);

        Entity bullet = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(bullet, TransformComponent{ startPos });
        gCoordinator.AddComponent(bullet, MeshComponent{ ShapeBuilder::CreateCubeScales({0.1, 0.1, 0.1},0.2f, 1, 0, 0) });
        gCoordinator.AddComponent(bullet, ColliderComponent{ 0.5f, true, false });
        gCoordinator.AddComponent(bullet, ProjectileComponent{ velocity, 10, ownerTeam, 2.0f });
        gCoordinator.AddComponent(bullet, ArcAnimComponent{ startPos, targetTrans.Pos, 2.0f, 0.8f, 0.0f });
        //gCoordinator.AddComponent(bullet, FactionComponent{ ownerTeam });
         gCoordinator.AddComponent(bullet, StatComponent{1 , 1, 20, ownerTeam});
 
    }

    void SpawnCatapultRock(vec3d startPos, Entity targetID, int ownerTeam)
    {
        if (!gCoordinator.HasComponent<TransformComponent>(targetID)) return;
        auto& targetTrans = gCoordinator.GetComponent<TransformComponent>(targetID);
        vec3d dir = Engine3D::Vector_Normalise(Engine3D::Vector_Sub(targetTrans.Pos, startPos));
        vec3d velocity = Engine3D::Vector_Mul(dir, 10.0f);

    
        Entity rock = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(rock, TransformComponent{ startPos });
        gCoordinator.AddComponent(rock, MeshComponent{ ShapeBuilder::CreateHexagonScales({0.4, 0.4, 0.4},0.2f, 0.2f, 0.2f, 0.2f) });
        gCoordinator.AddComponent(rock, ColliderComponent{ 0.5f, true, false }); // Larger collider = easier to hit
        
        ProjectileComponent rockProj;
        rockProj.velocity = velocity;
        rockProj.damage = 20;
        rockProj.ownerTeamId = ownerTeam;
        rockProj.lifetime = 1.8f;
        rockProj.splashRadius = 2.0f; // AoE damage within 4 units of impact!
        gCoordinator.AddComponent(rock, rockProj);
        
        gCoordinator.AddComponent(rock, ArcAnimComponent{ startPos, targetTrans.Pos, 5.0f, 1.5f, 0.0f });
        gCoordinator.AddComponent(rock, StatComponent{1 , 1, 50, ownerTeam});
    }
};