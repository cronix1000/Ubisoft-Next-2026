#pragma once

struct FactoryComponent {
    float productionTimer = 0.0f;
    float productionInterval = 1.0f; // Produce gold every 1 second
    int goldPerTick = 5;
};