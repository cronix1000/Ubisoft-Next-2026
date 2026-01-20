#pragma once
#include "System.h"
#include "Coordinator.h"
#include "ParticleComponent.h"

extern Coordinator gCoordinator;

class ParticleSystem : public System
{
public:
    void Update(float dt)
    {
        std::vector<Entity> entitiesToDestroy;
        float dtSeconds = dt / 1000.0f; // Convert ms to seconds

        for (auto const& entity : mEntities)
        {
            auto& particle = gCoordinator.GetComponent<ParticleComponent>(entity);
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& mesh = gCoordinator.GetComponent<MeshComponent>(entity);

            // 1. Move Particle
            transform.Pos.x += particle.velocity.x * dtSeconds;
            transform.Pos.y += particle.velocity.y * dtSeconds;
            transform.Pos.z += particle.velocity.z * dtSeconds;

            // 2. Update Lifetime
            particle.lifetime -= dtSeconds;

            // 3. Scale Animation (Shrink over time)
            float lifeRatio = particle.lifetime / particle.maxLifetime;
            float currentScale = particle.endScale + (particle.startScale - particle.endScale) * lifeRatio;
            
            // Apply scale to mesh (assuming simple cube mesh for particles)
            

            if (particle.lifetime <= 0.0f)
            {
                entitiesToDestroy.push_back(entity);
            }
        }

        // 4. Cleanup Dead Particles
        for (auto entity : entitiesToDestroy)
        {
            gCoordinator.DestroyEntity(entity);
        }
    }
};