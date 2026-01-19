
#include <iostream>
#include <math.h> 
#include <algorithm>
#include <vector>
#include <list>
#include "../ContestAPI/app.h"
#include "ShapeFactory.h"

#include "ThreeDVisualiser.h"
#include "3DRenderSystem.h"
#include "UIRenderSystem.h"
#include "MeshBuilder.h"
#include "UIButtonSystem.h"
#include "UIButton.h"
#include <freeglut_config.h>
#include "BuilderComponent.h"
#include "StatComponent.h"
#include "UnitSystem.h"
#include "SquadComponent.h"
#include "FactionComponent.h"
#include "SquadMemberComponent.h"
#include "CollisionSystem.h"
#include "PlayerControlSystem.h"
#include "UnitComponent.h"
#include "SquadSystem.h"           
#include "AnimationSystem.h"      
#include "ArcAnimComponent.h"
#include "ProjectileSystem.h"
#include "ProjectileComponent.h"
#include "UnitType.h"
#include "GoldComponent.h"
#include "ProductionSystem.h"
#include "ResourceSystem.h"
#include "GoldDepositComponent.h"
#include "UIProgressBarSystem.h"
#include "UIProgressBar.h"

using namespace Engine3D;

Coordinator gCoordinator;
Entity playerGold; // Holds our Score
Entity mouseCursor; // Holds our "Ghost" builder state
Entity btnScore;
Entity btnBuild;
Entity btnSpawnUnit;
Entity btnSpawnMelee;
Entity btnSpawnRanged;
Entity btnSpawnCatapult;
Entity playerUnit;
std::shared_ptr<SquadSystem> squadSystem;       
std::shared_ptr<AnimationSystem> animationSystem;
std::shared_ptr<UnitSystem> unitSystem; 
std::shared_ptr<CollisionSystem> collisionSystem;

std::shared_ptr<Render3DSystem> render3D;
std::shared_ptr<UIRenderSystem> renderUI;
std::shared_ptr<UIButtonSystem> renderButtonUI;
std::shared_ptr<UIProgressBarSystem> progressBarSystem;
std::shared_ptr<PlayerControlSystem> playerSystem;
std::shared_ptr<ProjectileSystem> projectileSystem;

std::shared_ptr<ResourceSystem> resourceSystem;
std::shared_ptr<ProductionSystem> productionSystem;
//------------------------------------------------------------------------
// GLOBAL STATE VARIABLES
//------------------------------------------------------------------------
mesh meshCube;
mat4x4 matProj;
vec3d vCamera;
vec3d vLookDir = { 0, 0, 1 };
vec3d vFocusPoint = { 0, 0, 0 };
float fYaw = 0.0f;
float fTheta = 0.0f;
bool isMousePressed;
bool isRightPressed;
bool wasRightPressed = false;
const float playerSpeed = 12;


vec3d GetIsoWorldCoordinates(float mouseX, float mouseY)
{
    // 1. Get NDC (Keep your previous fix)
    float ndc_x = mouseX;
    float ndc_y = mouseY;

    if (abs(mouseX) > 1.0f || abs(mouseY) > 1.0f)
    {
        ndc_x = (2.0f * mouseX / (float)APP_VIRTUAL_WIDTH) - 1.0f;
        ndc_y = 1.0f - (2.0f * mouseY / (float)APP_VIRTUAL_HEIGHT);
    }

    // 2. Manual Unprojection (Fixes the "Barely Moving" bug)
    // We explicitly scale NDC by the camera's Zoom dimensions.
    // These values MUST match what you set in Init()
    float zoom = 15.0f;
    float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;

    float viewWidth = zoom * aspectRatio;
    float viewHeight = zoom;

    // Convert NDC (-1 to 1) directly to Camera View Space
    // Range becomes [-7.5 to 7.5] instead of [-0.05 to 0.05]
    float viewX = ndc_x * (viewWidth / 2.0f);
    float viewY = ndc_y * (viewHeight / 2.0f);

    // 3. Transform View Space -> World Space
    // We create the Camera Matrix (transform from Camera to World)
    vec3d vUp = { 0.0f, 1.0f, 0.0f };
    mat4x4 matCamera = Matrix_PointAt(vCamera, vFocusPoint, vUp);

    // Create a Ray in View Space (Straight line forward from the camera plane)
    vec3d rayStartView = { viewX, viewY, -10.0f }; // Near point
    vec3d rayEndView = { viewX, viewY,  50.0f }; // Far point

    // Convert to World Space
    vec3d rayStart = Matrix_MultiplyVector(matCamera, rayStartView);
    vec3d rayEnd = Matrix_MultiplyVector(matCamera, rayEndView);

    // 4. Intersect with Ground (Plane Y = 0)
    vec3d rayDir = Vector_Normalise(Vector_Sub(rayEnd, rayStart));

    // Prevent divide by zero if looking perfectly horizontal
    if (abs(rayDir.y) < 0.001f) return { 0,0,0 };

    // t = (TargetY - StartY) / DirY
    float t = (0.0f - rayStart.y) / rayDir.y;

    // Calculate intersection
    vec3d worldPos = Vector_Add(rayStart, Vector_Mul(rayDir, t));

    // Lock Y to 0 exactly
    worldPos.y = 0.0f;

    return worldPos;
}


mesh CreateScaledWarrior(float scale) {
    mesh raw = ShapeBuilder::CreateWarrior();
    mesh finalMesh;
    // Add the raw mesh to the final mesh with a scale applied
    ShapeBuilder::AddMesh(finalMesh, raw, { 0,0,0 }, { scale, scale, scale });
    return finalMesh;
}

mesh CreateEnemyWarrior(float scale) {
	   mesh raw = ShapeBuilder::CreateEnemyWarrior();
    mesh finalMesh;
    // Add the raw mesh to the final mesh with a scale applied
    ShapeBuilder::AddMesh(finalMesh, raw, { 0,0,0 }, { scale, scale, scale });
    return finalMesh;
}

    void SpawnEnemySquad(int count, vec3d position)
    {
        // 1. Create the Squad Leader (The "Brain")
        Entity leader = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(leader, TransformComponent{ position });
        // Team ID 1 = Enemy
        gCoordinator.AddComponent(leader, SquadComponent{ 1, position, 0, 0, 4.0f });

		float meleeWeight = 0.7f;
		float rangedWeight = 0.2f;
		float catapultWeight = 0.1f;
		float scale = 0.3f;

        // 2. Create the Members (The "Grunts")
        for (int i = 0; i < count; i++)
        {
            Entity grunt = gCoordinator.CreateEntity();

            // Start them near the leader
            vec3d spawnPos = { position.x + (rand() % 10) / 10.0f, 0, position.z + (rand() % 10) / 10.0f };

            gCoordinator.AddComponent(grunt, TransformComponent{ spawnPos });
			float roll = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			UnitComponent::UnitType unitType;
			if (roll < meleeWeight) {
				unitType = UnitComponent::UnitType::meleeGrunt;
				gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 15, 0});
				scale = 0.3f;
			}
			else if (roll < meleeWeight + rangedWeight) {
				unitType = UnitComponent::UnitType::Ranged;
				gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 0, 0 });
				scale = 0.25f;
			}
			else {
				unitType = UnitComponent::UnitType::Catapult;
				gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 0, 0 });
				scale = 0.4f;
			}
            gCoordinator.AddComponent(grunt, UnitComponent{ unitType, spawnPos, false, 5.0f, false });
            gCoordinator.AddComponent(grunt, MeshComponent{ CreateEnemyWarrior(scale) });
            gCoordinator.AddComponent(grunt, FactionComponent{ 1 }); 
            gCoordinator.AddComponent(grunt, ColliderComponent{scale * 0.5f});

            // IMPORTANT: Add SquadMember pointing to the Leader
            gCoordinator.AddComponent(grunt, SquadMemberComponent{ leader, {0,0,0} });

            // Add AIComponent for shooting, but AISystem will skip 'Wander' because of SquadMemberComponent
            gCoordinator.AddComponent(grunt, AIComponent{ AIComponent::Type::Chaser });
        }

        // 3. Calculate initial offsets immediately
        // (Assuming you have access to the system instance)
        squadSystem->RecalculateFormation(leader);
    }
    void SpawnPlayerUnit(vec3d position, UnitComponent::UnitType type, float scale = 0.3f) {
        Entity grunt = gCoordinator.CreateEntity();

        vec3d spawnPos = { position.x + (rand() % 10) / 10.0f, 0, position.z + (rand() % 10) / 10.0f };

        gCoordinator.AddComponent(grunt, TransformComponent{ spawnPos });
        gCoordinator.AddComponent(grunt, FactionComponent{ 0 });
        gCoordinator.AddComponent(grunt, ColliderComponent{scale * 0.5f});
        switch (type) {
        case UnitComponent::UnitType::meleeGrunt:
            gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 15, 0 });
            scale = 0.3f;
            break;
        case UnitComponent::UnitType::Ranged:
            gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 0, 0 });
            scale = 0.5f;
            break;
        case UnitComponent::UnitType::Catapult:
            gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 0, 0 });
                scale = 0.8f;
            break;
        }
        gCoordinator.AddComponent(grunt, MeshComponent{ CreateScaledWarrior(scale) }); // Smaller player units



        gCoordinator.AddComponent(grunt, UnitComponent{ type, spawnPos, false, playerSpeed, false });

        gCoordinator.AddComponent(grunt, SquadMemberComponent{ playerUnit, {0,0,0} });

        gCoordinator.AddComponent(grunt, AIComponent{ AIComponent::Type::Wander });
        squadSystem->RecalculateFormation(playerUnit);
    }
    void SpawnPlayerSquad(int count, vec3d position)
{

    Entity leader = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(leader, TransformComponent{ position });
    gCoordinator.AddComponent(leader, SquadComponent{ 0, position, 0, 0, 10.0f }); // Team 0 = Player

    playerUnit = leader; 

    // 2. Create the Members (The units you see)
    for (int i = 0; i < count; i++)
    {
        SpawnPlayerUnit(position, UnitComponent::UnitType::Ranged);
    }
}

//------------------------------------------------------------------------
// Called before first update. Do any initial setup here.
//------------------------------------------------------------------------
void Init()
    {
        gCoordinator.Init();

        // 1. REGISTER COMPONENTS
        gCoordinator.RegisterComponent<TransformComponent>();
        gCoordinator.RegisterComponent<MeshComponent>();
        gCoordinator.RegisterComponent<UILabel>();
        gCoordinator.RegisterComponent<UIButton>();
		gCoordinator.RegisterComponent<UIProgressBar>();
        gCoordinator.RegisterComponent<StatComponent>();
        gCoordinator.RegisterComponent<BuilderComponent>();

        // NEW LOGIC COMPONENTS
        gCoordinator.RegisterComponent<UnitComponent>();
        gCoordinator.RegisterComponent<FactionComponent>();
        gCoordinator.RegisterComponent<AIComponent>();
        gCoordinator.RegisterComponent<ColliderComponent>();
        gCoordinator.RegisterComponent<ProjectileComponent>();
        gCoordinator.RegisterComponent<SquadComponent>();       
        gCoordinator.RegisterComponent<SquadMemberComponent>(); 
        gCoordinator.RegisterComponent<ArcAnimComponent>();
		gCoordinator.RegisterComponent<GoldComponent>();

		gCoordinator.RegisterComponent<GoldDepositComponent>();
        gCoordinator.RegisterComponent<FactoryComponent>();

        // NOTE: We removed SquadComponent/SquadMemberComponent to fix "Cohesion" confusion.
        // We now rely on PlayerControlSystem and AISystem.

        // 2. SETUP SYSTEMS
        render3D = gCoordinator.RegisterSystem<Render3DSystem>();
        Signature sig3D;
        sig3D.set(gCoordinator.GetComponentType<TransformComponent>());
        sig3D.set(gCoordinator.GetComponentType<MeshComponent>());
        gCoordinator.SetSystemSignature<Render3DSystem>(sig3D);

        renderUI = gCoordinator.RegisterSystem<UIRenderSystem>();
        Signature sigUI;
        sigUI.set(gCoordinator.GetComponentType<UILabel>());
        gCoordinator.SetSystemSignature<UIRenderSystem>(sigUI);

        renderButtonUI = gCoordinator.RegisterSystem<UIButtonSystem>();
        Signature sigBtton;
        sigBtton.set(gCoordinator.GetComponentType<UIButton>());
        gCoordinator.SetSystemSignature<UIButtonSystem>(sigBtton);

        unitSystem = gCoordinator.RegisterSystem<UnitSystem>();
        Signature sigUnit;
        sigUnit.set(gCoordinator.GetComponentType<TransformComponent>());
        sigUnit.set(gCoordinator.GetComponentType<UnitComponent>());
        gCoordinator.SetSystemSignature<UnitSystem>(sigUnit);

        collisionSystem = gCoordinator.RegisterSystem<CollisionSystem>();
        Signature sigCol;
        sigCol.set(gCoordinator.GetComponentType<TransformComponent>());
        sigCol.set(gCoordinator.GetComponentType<ColliderComponent>());
		sigCol.set(gCoordinator.GetComponentType<FactionComponent>());
        sigCol.set(gCoordinator.GetComponentType<StatComponent>());
        gCoordinator.SetSystemSignature<CollisionSystem>(sigCol);


        squadSystem = gCoordinator.RegisterSystem<SquadSystem>(); // NEW
   
            Signature sig;
            sig.set(gCoordinator.GetComponentType<TransformComponent>());

            gCoordinator.SetSystemSignature<SquadSystem>(sig);


        playerSystem = gCoordinator.RegisterSystem<PlayerControlSystem>();

        Signature sigPlayer;
        sigPlayer.set(gCoordinator.GetComponentType<UnitComponent>());
        gCoordinator.SetSystemSignature<PlayerControlSystem>(sigPlayer);

        animationSystem = gCoordinator.RegisterSystem<AnimationSystem>(); // NEW
        {
            Signature sig;
            sig.set(gCoordinator.GetComponentType<ArcAnimComponent>());
            gCoordinator.SetSystemSignature<AnimationSystem>(sig);
        }


        projectileSystem = gCoordinator.RegisterSystem<ProjectileSystem>();
        {
            Signature sig;
            sig.set(gCoordinator.GetComponentType<ProjectileComponent>());
            sig.set(gCoordinator.GetComponentType<TransformComponent>());
            gCoordinator.SetSystemSignature<ProjectileSystem>(sig);
        }

        resourceSystem = gCoordinator.RegisterSystem<ResourceSystem>();
        {
            Signature sig;
            sig.set(gCoordinator.GetComponentType<TransformComponent>());
            sig.set(gCoordinator.GetComponentType<GoldDepositComponent>());
            gCoordinator.SetSystemSignature<ResourceSystem>(sig);
        }

		progressBarSystem = gCoordinator.RegisterSystem<UIProgressBarSystem>();
		{
			Signature sig;
			sig.set(gCoordinator.GetComponentType<UIProgressBar>());
			gCoordinator.SetSystemSignature<UIProgressBarSystem>(sig);
		}

        productionSystem = gCoordinator.RegisterSystem<ProductionSystem>();
        {
            Signature sig;
            sig.set(gCoordinator.GetComponentType<FactoryComponent>());
            gCoordinator.SetSystemSignature<ProductionSystem>(sig);
        }
        // 3. CREATE ENTITIES

        // -- Ground --
        Entity ground = gCoordinator.CreateEntity();
        mesh groundMesh = ShapeBuilder::CreatePlane(500.0f, 0.2f, 0.5f, 0.2f); // Large plane
        gCoordinator.AddComponent(ground, TransformComponent{ {0, -0.1f, 0} });
        gCoordinator.AddComponent(ground, MeshComponent{ groundMesh });

        // -- Buildings --
        Entity factory = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(factory, TransformComponent{ {5, 0, 5} });
        gCoordinator.AddComponent(factory, MeshComponent{ ShapeBuilder::CreateFactoryUnit() });

        // -- Stats & UI --
for (int i = 0; i < 20; i++) {
            Entity goldChunk = gCoordinator.CreateEntity();
            // Random Pos
            float gx = (rand() % 80 - 40) * 1.0f;
            float gz = (rand() % 80 - 40) * 1.0f;
            
            gCoordinator.AddComponent(goldChunk, TransformComponent{ {gx, 0, gz} });
            
            // Yellow Cube
            mesh chunkMesh = ShapeBuilder::CreateCube(1.0f, 0.8f, 0.0f, false);
            gCoordinator.AddComponent(goldChunk, MeshComponent{ chunkMesh });
            gCoordinator.AddComponent(goldChunk, ColliderComponent{ 1.0f });
            gCoordinator.AddComponent(goldChunk, GoldDepositComponent{});
        }

        playerGold = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(playerGold, GoldComponent{ 0 });
        gCoordinator.AddComponent(playerGold, UILabel{ 10, 6, "Gold: 0", 1, 1, 1 });

        btnScore = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnScore, UIButton{ 10, 6, 150, 40, "Add Score", 0.2f, 0.6f, 0.2f });

        btnBuild = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnBuild, UIButton{ 50, 100, 150, 40, "Build Factory", 0.2f, 0.2f, 0.8f });

        btnSpawnUnit = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnSpawnUnit, UIButton{ 50, 150, 150, 40, "Spawn Unit", 0.7f, 0.2f, 0.2f });

        // Unit cards at bottom center
        float cardWidth = 120.0f;
        float cardHeight = 80.0f;
        float cardSpacing = 20.0f;
        float centerX = APP_VIRTUAL_WIDTH / 2.0f;
        float bottomY = APP_VIRTUAL_HEIGHT - cardHeight - 20.0f;
        
        // Melee Grunt - 10 gold
        btnSpawnMelee = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnSpawnMelee, UIButton{
            centerX - (cardWidth * 1.5f + cardSpacing),
            bottomY,
            cardWidth,
            cardHeight,
            "Melee\n10g",
            0.5f, 0.7f, 0.3f
        });
        
        // Ranged - 20 gold
        btnSpawnRanged = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnSpawnRanged, UIButton{
            centerX - (cardWidth / 2.0f),
            bottomY,
            cardWidth,
            cardHeight,
            "Ranged\n20g",
            0.3f, 0.5f, 0.8f
        });
        
        // Catapult - 50 gold
        btnSpawnCatapult = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnSpawnCatapult, UIButton{
            centerX + (cardWidth / 2.0f + cardSpacing),
            bottomY,
            cardWidth,
            cardHeight,
            "Catapult\n50g",
            0.8f, 0.3f, 0.3f
        });

        mouseCursor = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(mouseCursor, TransformComponent{ {0,0,0} });

        // 4. SPAWN ENEMIES
        SpawnEnemySquad(50.0f, {-10, 0, 5}); // Use the new group spawner
		SpawnEnemySquad(35.0f, {-10, 0, -50});
		SpawnEnemySquad(20.0f, {15, 0, -15});
        SpawnPlayerSquad(100, { 0, 0, 0 });

        // 5. CAMERA SETUP (The Fix for Black Screen)
        float zoom = 15.0f;
        float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;
        matProj = Engine3D::Matrix_MakeOrthographic(zoom * aspectRatio, zoom, -500.0f, 5000.0f);

        fYaw = 0.785398f;
        fTheta = 0.615472f;
        vFocusPoint = { 0, 0, 0 };

        // --- CALCULATE CAMERA POSITION ONCE ---
        mat4x4 matPitch = Matrix_MakeRotationX(fTheta);
        mat4x4 matYaw = Matrix_MakeRotationY(fYaw);
        mat4x4 matRot = Matrix_MultiplyMatrix(matPitch, matYaw);
        vec3d vOffset = { 0.0f, 0.0f, -20.0f }; // -20 Distance
        vOffset = Matrix_MultiplyVector(matRot, vOffset);
        vCamera = Vector_Add(vFocusPoint, vOffset);
        // --------------------------------------

        isMousePressed = false;
        isRightPressed = false;
    }
//---------------------------------------------------------------------
// Update your simulation here. 
//------------------------------------------------------------------------
void Update(const float deltaTime)
{
    float screenCenterX = (float)APP_VIRTUAL_WIDTH / 2.0f;
    float screenCenterY = (float)APP_VIRTUAL_HEIGHT / 2.0f;

    // 1. INPUT HANDLING
    vec3d worldCenter = GetIsoWorldCoordinates(screenCenterX, screenCenterY);
    float mouseX, mouseY;
    App::GetMousePos(mouseX, mouseY);
    float mouseYUI = APP_VIRTUAL_HEIGHT - mouseY;

    isMousePressed = App::IsMousePressed(GLUT_LEFT_BUTTON);
    
   
    bool isRightDown = App::IsMousePressed(GLUT_RIGHT_BUTTON);
    bool isRightClicked = isRightDown && !wasRightPressed; 
    wasRightPressed = isRightDown;                         
    isRightPressed = isRightDown;                         
 

    // Update Systems
    unitSystem->Update(deltaTime);
    collisionSystem->Update(deltaTime);
    squadSystem->Update(deltaTime);       
    animationSystem->Update(deltaTime);
    projectileSystem->Update(deltaTime);
    
    // resourceSystem->Update(deltaTime);
    
    // Updates gold production
    productionSystem->Update(deltaTime / 1000.0f, playerGold);

    // UI Updates
    auto& gold = gCoordinator.GetComponent<GoldComponent>(playerGold);
    auto& goldLabel = gCoordinator.GetComponent<UILabel>(playerGold);
    
    auto& btnMeleeUI = gCoordinator.GetComponent<UIButton>(btnSpawnMelee);
    auto& btnRangedUI = gCoordinator.GetComponent<UIButton>(btnSpawnRanged);
    auto& btnCatapultUI = gCoordinator.GetComponent<UIButton>(btnSpawnCatapult);
    
    btnMeleeUI.isDisabled = (gold.gold < 10);
    btnRangedUI.isDisabled = (gold.gold < 20);
    btnCatapultUI.isDisabled = (gold.gold < 50);

    // 2. HANDLE UI CLICKS (Left Click)
    Entity clickedID = renderButtonUI->UpdateInput(mouseX, mouseYUI, isMousePressed);
    
    if (clickedID == btnSpawnUnit)
    {
        Entity unit = gCoordinator.CreateEntity();
        mesh warriorMesh = CreateScaledWarrior(0.3f); 
        SpawnPlayerUnit(worldCenter, UnitComponent::UnitType::meleeGrunt);
    }
    
    if (clickedID == btnSpawnMelee && gold.gold >= 10) {
        SpawnPlayerUnit(worldCenter, UnitComponent::UnitType::meleeGrunt);
        gold.gold -= 10;
        goldLabel.text = "Gold: " + std::to_string(gold.gold);
    }
    
    if (clickedID == btnSpawnRanged && gold.gold >= 20) {
        SpawnPlayerUnit(worldCenter, UnitComponent::UnitType::Ranged);
        gold.gold -= 20;
        goldLabel.text = "Gold: " + std::to_string(gold.gold);
    }
    
    if (clickedID == btnSpawnCatapult && gold.gold >= 50) {
        SpawnPlayerUnit(worldCenter, UnitComponent::UnitType::Catapult);
        gold.gold -= 50;
        goldLabel.text = "Gold: " + std::to_string(gold.gold);
    }

    if (clickedID == btnScore) {
        gold.gold += 10;
        goldLabel.text = "Gold: " + std::to_string(gold.gold);
    }

    if (clickedID == btnBuild) {
        // Enter Building Mode
        gCoordinator.AddComponent(mouseCursor, BuilderComponent{ 1, true });
        mesh factoryGhost = ShapeBuilder::CreateFactoryUnit();
        gCoordinator.AddComponent(mouseCursor, MeshComponent{ factoryGhost });
    }

    // 3. HANDLE BUILDING LOGIC (Building Mode)
    if (gCoordinator.HasComponent<BuilderComponent>(mouseCursor))
    {
        // A. Move Ghost
        vec3d worldPos = GetIsoWorldCoordinates(mouseX, mouseY);
        auto& trans = gCoordinator.GetComponent<TransformComponent>(mouseCursor);
        trans.Pos.x = worldPos.x;
        trans.Pos.z = worldPos.z;
        trans.Pos.y = 0.0f; 

        // B. Check Validity (Gold Chunk)
        bool canBuild = false;
        Entity targetGoldChunk = -1;

        for (auto const& entity : resourceSystem->mEntities) {
            auto& goldTrans = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& deposit = gCoordinator.GetComponent<GoldDepositComponent>(entity);

            if (deposit.occupied) continue;

            float dist = Engine3D::Vector_Distance(trans.Pos, goldTrans.Pos);
            if (dist < 2.0f) {
                canBuild = true;
                targetGoldChunk = entity;
                break;
            }
        }

        // C. Update Visuals
        auto& ghostMeshComp = gCoordinator.GetComponent<MeshComponent>(mouseCursor);
        ghostMeshComp.mesh = ShapeBuilder::CreateFactoryUnit(); 

        if (canBuild) {
            // Normal color indicates valid
        } else {
            // Blue Tint indicates invalid
            ShapeBuilder::TintMesh(ghostMeshComp.mesh, 0.2f, 0.2f, 1.0f);
        }

        // D. Place Factory (Right Click)
        // FIX: Use isRightClicked here so we don't accidentally build when clicking UI
        if (isMousePressed && canBuild)
        {
            Entity newFactory = gCoordinator.CreateEntity();
            mesh factoryMesh = ShapeBuilder::CreateFactoryUnit();
            gCoordinator.AddComponent(newFactory, TransformComponent{ {worldPos.x, 0, worldPos.z} });
            gCoordinator.AddComponent(newFactory, MeshComponent{ factoryMesh });
            gCoordinator.AddComponent(newFactory, FactoryComponent{}); 

            // Add Health/Faction so enemies can attack it
            gCoordinator.AddComponent(newFactory, FactionComponent{ 0 }); // Team 0
            gCoordinator.AddComponent(newFactory, StatComponent{ 1000, 1000, 0, 0 }); // Health
            gCoordinator.AddComponent(newFactory, ColliderComponent{ 1.0f });

            // FIX: Removed UIProgressBar due to syntax error and missing definition
            gCoordinator.AddComponent(newFactory, UIProgressBar{  // Health Bar
				worldPos.x, worldPos.z, 10, 5,
				0.0f, 1.0f, 0.0f, // Green
				0 // progress
			}); 

            if (targetGoldChunk != -1) {
                auto& deposit = gCoordinator.GetComponent<GoldDepositComponent>(targetGoldChunk);
                deposit.occupied = true;
                deposit.linkedFactory = newFactory;
            }

            // Exit Building Mode
            gCoordinator.RemoveComponent<BuilderComponent>(mouseCursor);
            gCoordinator.RemoveComponent<MeshComponent>(mouseCursor); 
        }
    }

    // 4. PLAYER MOVEMENT (Camera follow)
    if (!gCoordinator.HasComponent<BuilderComponent>(mouseCursor))
    {
        playerSystem->Update(deltaTime);  
    }

    if (worldCenter.x != 0.0f || worldCenter.z != 0.0f)
    {
        Entity playerLeader = playerUnit;
        if (playerLeader != -1)
        {
            auto& trans = gCoordinator.GetComponent<TransformComponent>(playerLeader);
            trans.Pos = worldCenter;
        }
    }

    // 5. CAMERA CONTROLS
    float speed = playerSpeed * deltaTime / 1000.0f;
    if (App::IsKeyPressed(App::KEY_W)) vFocusPoint.z += speed;
    if (App::IsKeyPressed(App::KEY_S)) vFocusPoint.z -= speed;
    if (App::IsKeyPressed(App::KEY_A)) vFocusPoint.x -= speed;
    if (App::IsKeyPressed(App::KEY_D)) vFocusPoint.x += speed;

    if (vFocusPoint.z > MAP_LIMIT) vFocusPoint.z = MAP_LIMIT;
    if (vFocusPoint.z < -MAP_LIMIT) vFocusPoint.z = -MAP_LIMIT; 
    if (vFocusPoint.x > MAP_LIMIT) vFocusPoint.x = MAP_LIMIT;
    if (vFocusPoint.x < -MAP_LIMIT) vFocusPoint.x = -MAP_LIMIT;

    mat4x4 matPitch = Matrix_MakeRotationX(fTheta);
    mat4x4 matYaw = Matrix_MakeRotationY(fYaw);
    mat4x4 matRot = Matrix_MultiplyMatrix(matPitch, matYaw);
    vec3d vOffset = { 0.0f, 0.0f, -20.0f }; 
    vOffset = Matrix_MultiplyVector(matRot, vOffset);
    vCamera = Vector_Add(vFocusPoint, vOffset);
}
//------------------------------------------------------------------------
// Display calls here 
//------------------------------------------------------------------------
void Render()
{
	// Point camera at the focus point (the ground)
	vec3d vUp = { 0.0f, 1.0f, 0.0f };
	mat4x4 matCamera = Matrix_PointAt(vCamera, vFocusPoint, vUp);
	mat4x4 matView = Matrix_QuickInverse(matCamera);

	render3D->Draw(matView, matProj, vCamera);
	renderUI->Draw();
    renderButtonUI->Draw();
}
//-------------------------	-----------------------------------------------
// Shutdown
//------------------------------------------------------------------------
void Shutdown()
{
	// Clean up if necessary
}

