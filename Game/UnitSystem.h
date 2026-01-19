#pragma once
#include "System.h"
#include "Coordinator.h"
#include "Components.h" // Ensure UnitComponent is defined here

extern Coordinator gCoordinator;

class UnitSystem : public System
{
public:
    void Update(float dt)
    {
        // Static time tracker for the idle animation
        static float timeTracker = 0.0f;
        timeTracker += dt / 1000.0f;

        for (auto const& entity : mEntities)
        {
            auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& unit = gCoordinator.GetComponent<UnitComponent>(entity);

            // --- MOVEMENT LOGIC ---
            if (unit.isMoving)
            {
                // 1. Get vector to target
                vec3d diff = Engine3D::Vector_Sub(unit.targetPos, transform.Pos);
                diff.y = 0.0f; // Ignore height differences

                float dist = sqrtf(diff.x * diff.x + diff.z * diff.z);

                // 2. Calculate step size
                float moveStep = unit.speed * (dt / 1000.0f);

                // 3. OVERSHOOT CHECK (Fixes Glitching)
                // If we are closer than one step, just SNAP to the spot.
                if (dist <= moveStep || dist < 0.05f)
                {
                    transform.Pos.x = unit.targetPos.x;
                    transform.Pos.z = unit.targetPos.z;
                    unit.isMoving = false; // Stop moving
                }
                else
                {
                    // Normalize and Move
                    vec3d dir = { diff.x / dist, 0.0f, diff.z / dist };
                    transform.Pos.x += dir.x * moveStep;
                    transform.Pos.z += dir.z * moveStep;
                }
            }
            // --- IDLE ANIMATION (Only when stopped) ---
            else
            {
                // Simple Sway: Uses 'targetPos' as the anchor so they don't drift away
                float seed = (float)entity * 1.3f; // Randomize per unit
                float swayX = sinf(timeTracker * 2.0f + seed) * 0.1f;
                float swayZ = cosf(timeTracker * 1.7f + seed * 3.0f) * 0.1f;

                transform.Pos.x = unit.targetPos.x + swayX;
                transform.Pos.z = unit.targetPos.z + swayZ;
            }
        }
    }
};