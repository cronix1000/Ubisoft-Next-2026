#pragma once
#include "System.h"
#include "Coordinator.h"
#include "../ContestAPI/app.h" // Needed for App::Input
#include "ThreeDVisualiser.h"
#include "UnitComponent.h"

extern Coordinator gCoordinator;

// Forward declaration of your helper
vec3d GetIsoWorldCoordinates(float mouseX, float mouseY);

class PlayerControlSystem : public System
{
public:
    void Update(float dt)
    {
        // 1. Get Mouse Input
        float mx, my;
        App::GetMousePos(mx, my);
        bool isRightClick = App::IsMousePressed(GLUT_RIGHT_BUTTON);

        // 2. Issue Move Command
        if (isRightClick)
        {
            vec3d targetPos = GetIsoWorldCoordinates(mx, my);

            // Ignore clicks in the void
            if (targetPos.x == 0 && targetPos.z == 0) return;

            int groupCounter = 0;
            
            // Iterate only over units that belong to the PLAYER (Add a check if you have factions)
            for (auto const& entity : mEntities)
            {
                auto& unit = gCoordinator.GetComponent<UnitComponent>(entity);
                
                // Check if selected (and optionally check faction/team)
                if (unit.isSelected)
                {
                    // --- SPIRAL FORMATION MATH ---
                    float spacing = 0.8f; 
                    float radius = spacing * sqrtf(groupCounter);
                    float theta = groupCounter * 2.4f; // Golden Angle

                    float offsetX = radius * cosf(theta);
                    float offsetZ = radius * sinf(theta);

                    // COMMAND THE UNIT
                    // We simply set the data; UnitSystem will handle the physics next frame.
                    unit.targetPos = { targetPos.x + offsetX, 0, targetPos.z + offsetZ };
                    unit.isMoving = true;

                    groupCounter++;
                }
            }
        }
    }
};