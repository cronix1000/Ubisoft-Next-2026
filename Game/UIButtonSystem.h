#pragma once
#include "System.h"
#include "Coordinator.h"
#include "../ContestAPI/app.h"
#include "UIButton.h"
#include <string>

extern Coordinator gCoordinator;

class UIButtonSystem : public System
{
public:
    Entity UpdateInput(float mouseX, float mouseY, bool isMousePressed)
    {
        Entity clickedEntity = static_cast<Entity>(-1);

        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<UIButton>(entity)) continue;
            auto& btn = gCoordinator.GetComponent<UIButton>(entity);

            // Logic remains the same (Pixel Checks)
            btn.isClicked = false;
            bool collision = (mouseX >= btn.x && mouseX <= btn.x + btn.w &&
                mouseY >= btn.y && mouseY <= btn.y + btn.h);
            
            // Don't allow interaction with disabled buttons
            if (btn.isDisabled)
            {
                btn.isHovered = false;
                btn.isDown = false;
                continue;
            }
            
            btn.isHovered = collision;

            if (collision)
            {
                if (isMousePressed) btn.isDown = true;
                else if (btn.isDown)
                {
                    btn.isDown = false;
                    btn.isClicked = true;
                    clickedEntity = entity;
                }
            }
            else
            {
                if (!isMousePressed) btn.isDown = false;
            }
        }
        return clickedEntity;
    }

    void Draw()
    {
        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<UIButton>(entity)) continue;
            auto& btn = gCoordinator.GetComponent<UIButton>(entity);

            float r = btn.r;
            float g = btn.g;
            float b = btn.b;

            if (btn.isDisabled)
            {
                // Grayed out and desaturated
                r = g = b = 0.3f;
            }
            else if (btn.isDown) { r *= 0.5f; g *= 0.5f; b *= 0.5f; }
            else if (btn.isHovered) { r *= 1.2f; g *= 1.2f; b *= 1.2f; }

            float x1 = btn.x;
            float y1 = btn.y;
            float x2 = btn.x + btn.w;
            float y2 = btn.y + btn.h;

            // --- THE FIX ---
            // 1. Pass x1, y1 directly (No manual conversion).
            // 2. Pass Z = -1.0f. This is the "Near Plane". 
            //    It forces the UI to be drawn ON TOP of the 3D world.

            App::DrawTriangle(x1, y1, -1.0f, 1.0f, x2, y1, -1.0f, 1.0f, x1, y2, -1.0f, 1.0f, r, g, b, r, g, b, r, g, b, false);
            App::DrawTriangle(x2, y1, -1.0f, 1.0f, x2, y2, -1.0f, 1.0f, x1, y2, -1.0f, 1.0f, r, g, b, r, g, b, r, g, b, false);

            float textColor = btn.isDisabled ? 0.5f : 1.0f;
            App::Print(x1 + 10, y1 + (btn.h / 2) - 4, btn.text.c_str(), textColor, textColor, textColor);
        }
    }
};