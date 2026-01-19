
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
#include "AISystem.h"
#include "PlayerControlSystem.h"
#include "UnitComponent.h"
#include "SquadSystem.h"           
#include "AnimationSystem.h"      
#include "ArcAnimComponent.h"
#include "ProjectileSystem.h"
#include "ProjectileComponent.h"

using namespace Engine3D;

Coordinator gCoordinator;
Entity playerStats; // Holds our Score
Entity mouseCursor; // Holds our "Ghost" builder state
Entity btnScore;
Entity btnBuild;
Entity btnSpawnUnit;
Entity playerUnit;
std::shared_ptr<SquadSystem> squadSystem;       
std::shared_ptr<AnimationSystem> animationSystem;
std::shared_ptr<UnitSystem> unitSystem; 
std::shared_ptr<CollisionSystem> collisionSystem;
std::shared_ptr<AISystem> aiSystem;
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

void SpawnEnemySquad(float startX, float startZ) 
{
    // 1. Create the SQUAD LEADER (Virtual Entity)
    Entity squadEnt = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(squadEnt, TransformComponent{ {startX, 0, startZ} });
    gCoordinator.AddComponent(squadEnt, SquadComponent{ 1, {startX,0,startZ}, 5000.0f, 5.0f });
    // Note: No MeshComponent! It's invisible.

    // 2. Spawn SOLDIERS attached to this squad
    int squadSize = 5;
    for(int i=0; i<squadSize; i++) 
    {
        Entity soldier = gCoordinator.CreateEntity();
        
        // Calculate Formation Offset (Circle)
        float theta = i * (6.28f / squadSize); // 360 degrees / count
        float radius = 2.5f;
        vec3d offset = { cosf(theta)*radius, 0, sinf(theta)*radius };

        gCoordinator.AddComponent(soldier, TransformComponent{ {startX + offset.x, 0, startZ + offset.z} });
        gCoordinator.AddComponent(soldier, MeshComponent{ ShapeBuilder::CreateWarrior() });
        gCoordinator.AddComponent(soldier, FactionComponent{ 1 });
        gCoordinator.AddComponent(soldier, AIComponent{ AIComponent::Type::Wander }); // Just tagging it as AI        
        // LINK TO SQUAD
        gCoordinator.AddComponent(soldier, SquadMemberComponent{ squadEnt, {startX, 0, startZ} });
    }
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

	void SpawnEnemyGroup(float startX, float startZ, int count)
{
    for(int i = 0; i < count; i++)
    {
        Entity enemy = gCoordinator.CreateEntity();

        // Randomize start slightly
        float rX = (rand() % 20) / 10.0f; 
        float rZ = (rand() % 20) / 10.0f;

        gCoordinator.AddComponent(enemy, TransformComponent{ {startX + rX, 0, startZ + rZ} });
        gCoordinator.AddComponent(enemy, MeshComponent{ CreateEnemyWarrior(0.5f) }); // Use different color/mesh if possible
        gCoordinator.AddComponent(enemy, FactionComponent{ 1 }); // Enemy Team
		gCoordinator.AddComponent(enemy, AIComponent{ AIComponent::Type::Wander }); // Simple AI
		
        gCoordinator.AddComponent(enemy, UnitComponent{ {startX,0,startZ}, false, 5.0f, false });
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
        gCoordinator.SetSystemSignature<CollisionSystem>(sigCol);

        aiSystem = gCoordinator.RegisterSystem<AISystem>();
        Signature sigAI;
        sigAI.set(gCoordinator.GetComponentType<TransformComponent>());
        sigAI.set(gCoordinator.GetComponentType<AIComponent>());
        // aiSig.set(gCoordinator.GetComponentType<FactionComponent>()); // Optional safety
        gCoordinator.SetSystemSignature<AISystem>(sigAI);

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

        // -- Player Unit (Commandable) --
        Entity warrior = gCoordinator.CreateEntity();
        mesh warriorMesh = ShapeBuilder::CreateWarrior();
        gCoordinator.AddComponent(warrior, TransformComponent{ {0, 0, 0} });
        gCoordinator.AddComponent(warrior, MeshComponent{ warriorMesh });

        // FIX: Add UnitComponent so PlayerControlSystem can control it!
        // targetPos={0,0,0}, isMoving=false, speed=10.0f, isSelected=true
        gCoordinator.AddComponent(warrior, UnitComponent{ {0,0,0}, false, 10.0f, true });
        gCoordinator.AddComponent(warrior, FactionComponent{ 0 }); // Team 0 = Player

        // -- Buildings --
        Entity factory = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(factory, TransformComponent{ {5, 0, 5} });
        gCoordinator.AddComponent(factory, MeshComponent{ ShapeBuilder::CreateFactoryUnit() });

        // -- Stats & UI --
        playerStats = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(playerStats, StatComponent{ 0, 100 });
        gCoordinator.AddComponent(playerStats, UILabel{ 10, 6, "Score: 0", 1, 1, 1 });

        btnScore = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnScore, UIButton{ 10, 6, 150, 40, "Add Score", 0.2f, 0.6f, 0.2f });

        btnBuild = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnBuild, UIButton{ 50, 100, 150, 40, "Build Factory", 0.2f, 0.2f, 0.8f });

        btnSpawnUnit = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(btnSpawnUnit, UIButton{ 50, 150, 150, 40, "Spawn Unit", 0.7f, 0.2f, 0.2f });

        mouseCursor = gCoordinator.CreateEntity();
        gCoordinator.AddComponent(mouseCursor, TransformComponent{ {0,0,0} });

        // 4. SPAWN ENEMIES
        SpawnEnemyGroup(20.0f, 20.0f, 5); // Use the new group spawner

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
	aiSystem->Update(deltaTime);
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

        gCoordinator.AddComponent(unit, TransformComponent{ {5.0f + offsetX, 0, 5.0f + offsetZ} });
        gCoordinator.AddComponent(unit, MeshComponent{ warriorMesh });
        gCoordinator.AddComponent(unit, UnitComponent{}); // Default constructor sets defaults
    }
	if (clickedID == btnScore)
    {
        // SIMPLE INTERACTION: Direct Modification
        auto& stats = gCoordinator.GetComponent<StatComponent>(playerStats);
        stats.score += 10;

        // Update the Label too
        auto& label = gCoordinator.GetComponent<UILabel>(playerStats);
        label.text = "Score: " + std::to_string(stats.score);
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

