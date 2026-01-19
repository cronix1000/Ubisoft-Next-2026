#pragma once
#include "System.h"
#include "Coordinator.h"
#include "../ContestAPI/app.h"
#include "UIProgressBar.h"
#include <string>

extern Coordinator gCoordinator;

class UIProgressBarSystem : public System
{
public:
    void Draw()
    {
        for (auto const& entity : mEntities)
        {
            if (!gCoordinator.HasComponent<UIProgressBar>(entity)) continue;
            auto& progressBar = gCoordinator.GetComponent<UIProgressBar>(entity);

            float r = progressBar.r;
            float g = progressBar.g;
            float b = progressBar.b;


            
            float x1 = progressBar.x;
            float y1 = progressBar.y;
            float x2 = (progressBar.x + progressBar.width) * progressBar.progress;
            float y2 = progressBar.y + progressBar.height;

            App::DrawTriangle(x1, y1, -1.0f, 1.0f, x2, y1, -1.0f, 1.0f, x1, y2, -1.0f, 1.0f, r, g, b, r, g, b, r, g, b, false);
            App::DrawTriangle(x2, y1, -1.0f, 1.0f, x2, y2, -1.0f, 1.0f, x1, y2, -1.0f, 1.0f, r, g, b, r, g, b, r, g, b, false);
        }
    }
};