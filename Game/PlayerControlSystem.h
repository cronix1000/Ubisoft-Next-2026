#pragma once
#include "System.h"
#include "Coordinator.h"
#include "../ContestAPI/app.h" 
#include "ThreeDVisualiser.h"
#include "SquadSystem.h"

extern Coordinator gCoordinator;
extern vec3d GetIsoWorldCoordinates(float mouseX, float mouseY); 
extern std::shared_ptr<SquadSystem> squadSystem;

class PlayerControlSystem : public System
{
public:
    void Update(float dt)
    {

        // 1. SPACEBAR: SPLIT SQUAD
        static bool spacePressedLast = false;
        bool spacePressed = App::IsKeyPressed(App::KEY_SPACE); // Use char ' ' or VK_SPACE depending on API

        if (spacePressed && !spacePressedLast)
        {
            SplitSquad();
        }
        spacePressedLast = spacePressed;


    }

private:
    void SplitSquad()
    {
        // 1. Find the Player's Main Squad Leader
        Entity mainLeader = squadSystem->GetPlayerSquadLeader();
        if (mainLeader == -1) return;

        // 2. Collect all members of this squad
        std::vector<Entity> myUnits;
        for (auto const& entity : squadSystem->mEntities)
        {
            if (gCoordinator.HasComponent<SquadMemberComponent>(entity))
            {
                if (gCoordinator.GetComponent<SquadMemberComponent>(entity).squadId == mainLeader)
                {
                    myUnits.push_back(entity);
                }
            }
        }

        // 3. Only split if we have enough units
        if (myUnits.size() < 2) return;

        // 4. Create a NEW Squad Leader for the split group
        Entity newLeader = gCoordinator.CreateEntity();

        // Spawn the new leader slightly away from the player so they don't overlap
        auto& playerPos = gCoordinator.GetComponent<TransformComponent>(mainLeader).Pos;
        vec3d splitPos = { playerPos.x + 3.0f, playerPos.y, playerPos.z };

        gCoordinator.AddComponent(newLeader, TransformComponent{ splitPos });
        gCoordinator.AddComponent(newLeader, SquadComponent{ 0, splitPos, 0, 0, 0.0f }); // Speed 0 = Stationary

        // 5. Reassign HALF the units to the new leader
        int splitCount = myUnits.size() / 2;
        for (int i = 0; i < splitCount; i++)
        {
            auto& mem = gCoordinator.GetComponent<SquadMemberComponent>(myUnits[i]);
            mem.squadId = newLeader;
        }

        // 6. Recalculate formations for both groups so they look nice immediately
        squadSystem->RecalculateFormation(mainLeader);
        squadSystem->RecalculateFormation(newLeader);
    }
};