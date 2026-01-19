
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

using namespace Engine3D;

Coordinator gCoordinator;
Entity playerGold; // Holds our Score
Entity mouseCursor; // Holds our "Ghost" builder state
Entity btnScore;
Entity btnBuild;
Entity btnSpawnUnit;
Entity playerUnit;
std::shared_ptr<SquadSystem> squadSystem;       
std::shared_ptr<AnimationSystem> animationSystem;
std::shared_ptr<UnitSystem> unitSystem; 
std::shared_ptr<CollisionSystem> collisionSystem;

std::shared_ptr<Render3DSystem> render3D;
std::shared_ptr<UIRenderSystem> renderUI;
std::shared_ptr<UIButtonSystem> renderButtonUI;
std::shared_ptr<PlayerControlSystem> playerSystem;
std::shared_ptr<ProjectileSystem> projectileSystem;
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
				gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 15, 1 });
				scale = 0.3f;
			}
			else if (roll < meleeWeight + rangedWeight) {
				unitType = UnitComponent::UnitType::Ranged;
				gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 0, 1 });
				scale = 0.25f;
			}
			else {
				unitType = UnitComponent::UnitType::Catapult;
				gCoordinator.AddComponent(grunt, StatComponent{ 100, 100, 0, 1 });
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
        gCoordinator.AddComponent(grunt, MeshComponent{ CreateScaledWarrior(scale) }); // Smaller player units
        gCoordinator.AddComponent(grunt, FactionComponent{ 0 });
        gCoordinator.AddComponent(grunt, ColliderComponent{scale * 0.5f});

        gCoordinator.AddComponent(grunt, UnitComponent{ type, spawnPos, false, 8.0f, false });

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
        SpawnPlayerUnit(position, UnitComponent::UnitType::meleeGrunt);
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
        gCoordinator.SetSystemSignature<CollisionSystem>(sigCol);


        squadSystem = gCoordinator.RegisterSystem<SquadSystem>(); // NEW
        {
            Signature sig;
            sig.set(gCoordinator.GetComponentType<TransformComponent>());
            // SquadSystem filters internally, so Transform is the minimum
            gCoordinator.SetSystemSignature<SquadSystem>(sig);
        }
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
        playerGold = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(playerGold, GoldComponent{ 0 });
        gCoordinator.AddComponent(playerGold, UILabel{ 10, 6, "Gold: 0", 1, 1, 1 });

        btnScore = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnScore, UIButton{ 10, 6, 150, 40, "Add Score", 0.2f, 0.6f, 0.2f });

        btnBuild = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnBuild, UIButton{ 50, 100, 150, 40, "Build Factory", 0.2f, 0.2f, 0.8f });

        btnSpawnUnit = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnSpawnUnit, UIButton{ 50, 150, 150, 40, "Spawn Unit", 0.7f, 0.2f, 0.2f });

        mouseCursor = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(mouseCursor, TransformComponent{ {0,0,0} });

        // 4. SPAWN ENEMIES
        SpawnEnemySquad(20.0f, {5, 0, 5}); // Use the new group spawner
		SpawnEnemySquad(15.0f, {-10, 0, -10});
		SpawnEnemySquad(10.0f, {15, 0, -15});
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

    // Convert to World Position
    vec3d worldCenter = GetIsoWorldCoordinates(screenCenterX, screenCenterY);
float mouseX, mouseY;
    App::GetMousePos(mouseX, mouseY);
    float mouseYUI = APP_VIRTUAL_HEIGHT - mouseY;
     isMousePressed = App::IsMousePressed(GLUT_LEFT_BUTTON);
     isRightPressed = App::IsMousePressed(GLUT_RIGHT_BUTTON);
     bool isRightDown = App::IsMousePressed(GLUT_RIGHT_BUTTON);

     // "Click" happens only on the frame the button goes DOWN
     bool isRightClicked = isRightDown && !wasRightPressed;

     // Update "Was" for next frame
     wasRightPressed = isRightDown;
    unitSystem->Update(deltaTime);
	collisionSystem->Update(deltaTime);
    squadSystem->Update(deltaTime);      
    unitSystem->Update(deltaTime);    
    animationSystem->Update(deltaTime);
    projectileSystem->Update(deltaTime);

	// 1. HANDLE UI BUTTON CLICKS
	Entity clickedID = renderButtonUI->UpdateInput(mouseX, mouseYUI, isMousePressed);
	if (clickedID == btnSpawnUnit)
    {
        // SPAWN LOGIC
        Entity unit = gCoordinator.CreateEntity();
        
        // Use our helper to scale the warrior down (0.3 scale)
        mesh warriorMesh = CreateScaledWarrior(0.3f); 
        
        // Spawn at a default location (e.g., near the first factory)
        // Adding a small random offset so they don't spawn inside each other
        float offsetX = (rand() % 100) / 50.0f; 
        float offsetZ = (rand() % 100) / 50.0f;

        SpawnPlayerUnit(worldCenter, UnitComponent::UnitType::meleeGrunt);
    }
	if (clickedID == btnScore)
    {
        // SIMPLE INTERACTION: Direct Modification
        auto& gold = gCoordinator.GetComponent<GoldComponent>(playerGold);
        gold.gold += 10;

        // Update the Label too
        auto& label = gCoordinator.GetComponent<UILabel>(playerGold);
		label.text = "Gold: " + std::to_string(gold.gold);
    }

    if (clickedID == btnBuild)
    {
        // MODE INTERACTION: Enter "Building Mode"
        // We add the component to the cursor entity
        gCoordinator.AddComponent(mouseCursor, BuilderComponent{ 1, true });
        
        // Optional: Add a Ghost Mesh to the cursor so we see what we are building
        mesh factoryGhost = ShapeBuilder::CreateFactoryUnit();
        gCoordinator.AddComponent(mouseCursor, MeshComponent{ factoryGhost });
    }

    // 2. HANDLE BUILDING LOGIC (The "Mode")
    if (gCoordinator.HasComponent<BuilderComponent>(mouseCursor))
    {
        // We are in Building Mode!
        
        // A. Move Ghost to Mouse Position
        vec3d worldPos = GetIsoWorldCoordinates(mouseX, mouseY);
        auto& trans = gCoordinator.GetComponent<TransformComponent>(mouseCursor);
        trans.Pos.x = worldPos.x;
        trans.Pos.z = worldPos.z;
        trans.Pos.y = 0.0f; // Snap to floor

        // B. Check for Placement Click (Right Click to Place)
        if (isRightPressed)
        {
            // Spawn Real Entity
            Entity newFactory = gCoordinator.CreateEntity();
            mesh factoryMesh = ShapeBuilder::CreateFactoryUnit();
            gCoordinator.AddComponent(newFactory, TransformComponent{ {worldPos.x, 0, worldPos.z} });
            gCoordinator.AddComponent(newFactory, MeshComponent{ factoryMesh });

            // Exit Building Mode
            gCoordinator.RemoveComponent<BuilderComponent>(mouseCursor);
            gCoordinator.RemoveComponent<MeshComponent>(mouseCursor); // Remove ghost mesh
        }
    }

    if (!gCoordinator.HasComponent<BuilderComponent>(mouseCursor))
    {
     playerSystem->Update(deltaTime);  
	}



    // Update Position (Only if raycast hit something valid)
    if (worldCenter.x != 0.0f || worldCenter.z != 0.0f)
    {
        Entity playerLeader = playerUnit;
        if (playerLeader != -1)
        {
            auto& trans = gCoordinator.GetComponent<TransformComponent>(playerLeader);
            trans.Pos = worldCenter;
        }
    }


    float speed = 20.0f * deltaTime / 1000.0f;
    if (App::IsKeyPressed(App::KEY_W)) vFocusPoint.z += speed;
    if (App::IsKeyPressed(App::KEY_S)) vFocusPoint.z -= speed;
    if (App::IsKeyPressed(App::KEY_A)) vFocusPoint.x -= speed;
    if (App::IsKeyPressed(App::KEY_D)) vFocusPoint.x += speed;

    // Recalculate Camera
    mat4x4 matPitch = Matrix_MakeRotationX(fTheta);
    mat4x4 matYaw = Matrix_MakeRotationY(fYaw);
    mat4x4 matRot = Matrix_MultiplyMatrix(matPitch, matYaw);
    vec3d vOffset = { 0.0f, 0.0f, -20.0f }; // Distance
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

