#pragma once
#include <ThreeDVisualiser.h>
struct ArcAnimComponent {
    vec3d startPos;
    vec3d targetPos;
    float peakHeight;   // How high the arc goes
    float duration;     // How long to reach target (seconds)
    float elapsedTime;  // Timer
};