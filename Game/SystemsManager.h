#pragma once
#pragma once
#include <vector>
#include <functional>
#include "Coordinator.h" 
// A System is just a void function that takes the Registry and DeltaTime
using SystemFunc = std::function<void(Coordinator&, float)>;

class SystemManager {
private:
    std::vector<SystemFunc> updateSystems;
    std::vector<SystemFunc> renderSystems;

public:
    void AddUpdateSystem(SystemFunc sys) {
        updateSystems.push_back(sys);
    }

    // Register a system to run during rendering (Drawing)
    void AddRenderSystem(SystemFunc sys) {
        renderSystems.push_back(sys);
    }

    // Call this in your GameTest Update()
    void Update(Coordinator& reg, float dt) {
        for (auto& sys : updateSystems) {
            sys(reg, dt);
        }
    }

    // Call this in your GameTest Render()
    void Render(Coordinator& reg, float dt = 0.0f) {
        for (auto& sys : renderSystems) {
            sys(reg, dt);
        }
    }
};