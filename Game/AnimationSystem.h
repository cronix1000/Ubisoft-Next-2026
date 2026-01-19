#pragma once
#include "System.h"
#include "Coordinator.h"
#include "TransformComponent.h"
#include "ArcAnimComponent.h"
#include <vector> // Required for vector

extern Coordinator gCoordinator;

class AnimationSystem : public System
{
public:
    void Update(float dt)
    {
        // 1. Create a temporary list to store finished entities
        std::vector<Entity> toRemove;

        for (auto const& entity : mEntities)
        {
            auto& anim = gCoordinator.GetComponent<ArcAnimComponent>(entity);
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);

            anim.elapsedTime += dt / 1000.0f;

            float t = anim.elapsedTime / anim.duration;

            if (t >= 1.0f)
            {
                // Reached Target
                trans.Pos = anim.targetPos;

                // CRITICAL FIX: Do NOT remove component here. 
                // Add to list and remove LATER.
                toRemove.push_back(entity);
                continue;
            }

            // ... Interpolation Logic ...
            vec3d currentPos;
            currentPos.x = anim.startPos.x + (anim.targetPos.x - anim.startPos.x) * t;
            currentPos.z = anim.startPos.z + (anim.targetPos.z - anim.startPos.z) * t;

            float heightOffset = 4.0f * anim.peakHeight * t * (1.0f - t);

            float startY = anim.startPos.y;
            float endY = anim.targetPos.y;
            float linearY = startY + (endY - startY) * t;

            currentPos.y = linearY + heightOffset;

            trans.Pos = currentPos;
        }

        // 2. NOW it is safe to remove components
        for (auto entity : toRemove)
        {
            gCoordinator.RemoveComponent<ArcAnimComponent>(entity);
        }
    }
};