#pragma once
#include "GoldComponent.h"
#include "UILabel.h"
#include "FactoryComponent.h"
#include "Coordinator.h"
#include "UIProgressBar.h"
extern Coordinator gCoordinator;

class ProductionSystem : public System {
public:
    void Update(float dt, Entity playerGoldEntity) {
        for (auto const& entity : mEntities) {
            auto& factory = gCoordinator.GetComponent<FactoryComponent>(entity);
            factory.productionTimer += dt;

            if (factory.productionTimer >= factory.productionInterval) {
                factory.productionTimer = 0.0f;

                // Add gold to player
                if (playerGoldEntity != -1) {
                    auto& goldComp = gCoordinator.GetComponent<GoldComponent>(playerGoldEntity);
                    auto& label = gCoordinator.GetComponent<UILabel>(playerGoldEntity);
                    
                    goldComp.gold += factory.goldPerTick;
                    label.text = "Gold: " + std::to_string(goldComp.gold);
                }
            }
        }
    }
};