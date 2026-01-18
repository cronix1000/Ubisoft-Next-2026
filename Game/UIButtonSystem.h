#pragma once
#include "System.h"
#include "Coordinator.h"
#include "../ContestAPI/app.h"
#include "UIButton.h"

extern Coordinator gCoordinator;

class UIButtonSystem : public System
{
public:
    // Call this in your GameEngine::Update()
    // It returns the Entity ID of the clicked button, or -1 if nothing clicked.
    Entity UpdateInput(float mouseX, float mouseY, bool isMousePressed)
    {
        Entity clickedEntity = -1; // -1 means nothing clicked

        for (auto const& entity : mEntities)
        {
            auto& btn = gCoordinator.GetComponent<UIButton>(entity);

            // 1. Reset Click State (so it only triggers once per frame)
            btn.isClicked = false;

            // 2. Check Collision (AABB)
            bool collision = (mouseX >= btn.x && mouseX <= btn.x + btn.w &&
                              mouseY >= btn.y && mouseY <= btn.y + btn.h);

            // 3. Update Hover State
            btn.isHovered = collision;

            // 4. Handle Clicks
            if (collision)
            {
                if (isMousePressed)
                {
                    btn.isDown = true;
                }
                else if (btn.isDown) 
                {
                    // Mouse was released WHILE over the button -> CLICK!
                    btn.isDown = false;
                    btn.isClicked = true;
                    clickedEntity = entity; // Return this entity
                }
            }
            else
            {
                // Mouse left the button, cancel the press
                if (!isMousePressed) btn.isDown = false;
            }
        }
        return clickedEntity;
    }

    // Call this in GameEngine::Render()
    void Draw()
    {
        for (auto const& entity : mEntities)
        {
            auto& btn = gCoordinator.GetComponent<UIButton>(entity);

            // Determine Color based on State
            float r = btn.r; 
            float g = btn.g; 
            float b = btn.b;

            if (btn.isDown) { r *= 0.5f; g *= 0.5f; b *= 0.5f; }      // Darken when held
            else if (btn.isHovered) { r *= 1.2f; g *= 1.2f; b *= 1.2f; } // Brighten when hovered

            // Draw Background (2 Triangles)
            float x1 = btn.x; float y1 = btn.y;
            float x2 = btn.x + btn.w; float y2 = btn.y + btn.h;

            App::DrawTriangle(x1, y1, 0, 1, x2, y1, 0, 1, x1, y2, 0, 1, r, g, b, r, g, b, r, g, b, false);
            App::DrawTriangle(x2, y1, 0, 1, x2, y2, 0, 1, x1, y2, 0, 1, r, g, b, r, g, b, r, g, b, false);

            // Draw Text (Centered)
            // Note: Simple centering. Adjust '4' and '10' based on font size if needed.
            App::Print(x1 + 10, y1 + (btn.h / 2) - 4, btn.text.c_str(), 1, 1, 1);
        }
    }
};