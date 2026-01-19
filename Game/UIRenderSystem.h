#pragma once
#include "System.h"
#include <app.h>
#include "Coordinator.h"
#include "UILabel.h"
#include "freeglut_config.h"

extern Coordinator gCoordinator;

class UIRenderSystem : public System
{
public:
    void Draw()
    {
        for (auto const& entity : mEntities)
        {
            auto& label = gCoordinator.GetComponent<UILabel>(entity);

            // Draw Text
            App::Print(label.x, label.y, label.text.c_str(), label.r, label.g, label.b, GLUT_BITMAP_TIMES_ROMAN_24);


        }
    }
};