#pragma once
#include "System.h"
#include "Coordinator.h"
#include "ProjectileComponent.h" 

extern Coordinator gCoordinator;

class ProjectileSystem : public System
{
public:
    void Update(float dt){
        float deltaSeconds = dt / 1000.0f;
        std::vector<Entity> toRemove;
        for(auto& entity : mEntities)
        {
            auto& projectile = gCoordinator.GetComponent<ProjectileComponent>(entity);
            // Update position
            projectile.lifetime -= deltaSeconds;
            
            if(projectile.lifetime <= 0.0f)
            {
                toRemove.push_back(entity);
                continue;
            }
        }
        for(auto entity : toRemove)
        {

            gCoordinator.DestroyEntity(entity);
        }
    }
};