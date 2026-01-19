#pragma once
#include "ECSBase.h"
#include "Components.h"
#include "../ContestAPI/app.h"
#include "ThreeDVisualiser.h" 

using namespace Engine3D;

class UIProgressBarSystem : public System
{
public:
    // Accept Camera Matrices to calculate 3D -> 2D position
    void Draw( mat4x4& matView, mat4x4& matProj)
    {
        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<UIProgressBar>(entity)) continue;

            auto& bar = gCoordinator.GetComponent<UIProgressBar>(entity);

            float screenX = bar.x;
            float screenY = bar.y;

            // IF the entity has a Transform (like a Factory), calculate screen position
            if (gCoordinator.HasComponent<TransformComponent>(entity))
            {
                auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
                vec3d worldPos = transform.Pos;

                // Offset: Float the bar slightly above the unit (Y + 2.0f)
                worldPos.y += 2.0f;

                // 1. World -> View Space
                vec3d viewPos = Matrix_MultiplyVector(matView, worldPos);

                // 2. View -> Clip Space (NDC)
                vec3d clipPos = Matrix_MultiplyVector(matProj, viewPos);

                // 3. NDC -> Screen Coordinates
                // NDC is [-1, 1]. Screen is [0, Width].
                // We also flip Y because Screen 0 is Top, but NDC 1 is Top.
                screenX = (clipPos.x + 1.0f) * 0.5f * APP_VIRTUAL_WIDTH;
                screenY = (1.0f - clipPos.y) * 0.5f * APP_VIRTUAL_HEIGHT;

                // Center the bar horizontally
                screenX -= bar.width * 0.5f;
            }

            // Draw Background (Black Border)
            float padding = 1.0f;
            float bx1 = screenX - padding;
            float by1 = screenY - padding;
            float bx2 = screenX + bar.width + padding;
            float by2 = screenY + bar.height + padding;
            App::DrawTriangle(bx1, by1, -1.0f, 1.0f, bx2, by1, -1.0f, 1.0f, bx1, by2, -1.0f, 1.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);
            App::DrawTriangle(bx2, by1, -1.0f, 1.0f, bx2, by2, -1.0f, 1.0f, bx1, by2, -1.0f, 1.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, false);

            // Draw Foreground (Progress)
            float x1 = screenX;
            float y1 = screenY;

            // FIX: Correct math is x1 + (width * progress)
            float x2 = screenX + (bar.width * bar.progress);
            float y2 = screenY + bar.height;

            if (bar.progress > 0) {
                App::DrawTriangle(x1, y1, -1.0f, 1.0f, x2, y1, -1.0f, 1.0f, x1, y2, -1.0f, 1.0f, bar.r, bar.g, bar.b, bar.r, bar.g, bar.b, bar.r, bar.g, bar.b, false);
                App::DrawTriangle(x2, y1, -1.0f, 1.0f, x2, y2, -1.0f, 1.0f, x1, y2, -1.0f, 1.0f, bar.r, bar.g, bar.b, bar.r, bar.g, bar.b, bar.r, bar.g, bar.b, false);
            }
        }
    }
};