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
#include "FactoryComponent.h"

using namespace Engine3D;

//------------------------------------------------------------------------
// GLOBAL ENTITIES & SYSTEMS
//------------------------------------------------------------------------
Coordinator gCoordinator;

// UI Entities
Entity playerGold;
Entity mouseCursor;
Entity btnScore;
Entity btnBuild;
Entity btnSpawnUnit;
Entity btnSpawnMelee;
Entity btnSpawnRanged;
Entity btnSpawnCatapult;

// Logic Entities
Entity playerUnit;

// Systems
std::shared_ptr<SquadSystem> squadSystem;
std::shared_ptr<AnimationSystem> animationSystem;
std::shared_ptr<UnitSystem> unitSystem;
std::shared_ptr<CollisionSystem> collisionSystem;
std::shared_ptr<Render3DSystem> render3D;
std::shared_ptr<UIRenderSystem> renderUI;
std::shared_ptr<UIButtonSystem> renderButtonUI;
std::shared_ptr<PlayerControlSystem> playerSystem;
std::shared_ptr<ProjectileSystem> projectileSystem;
std::shared_ptr<UIProgressBarSystem> progressBarSystem;
std::shared_ptr<ResourceSystem> resourceSystem;
std::shared_ptr<ProductionSystem> productionSystem;

// Global State
mesh meshCube;
mat4x4 matProj;
vec3d vCamera;
vec3d vFocusPoint = { 0, 0, 0 };
float fYaw = 0.0f;
float fTheta = 0.0f;
bool isMousePressed;
bool isRightPressed;
bool wasRightPressed = false;
const float playerSpeed = 12;

//------------------------------------------------------------------------
// HELPER FUNCTIONS
//------------------------------------------------------------------------

vec3d GetIsoWorldCoordinates(float mouseX, float mouseY)
{
    float ndc_x = mouseX;
    float ndc_y = mouseY;

    if (abs(mouseX) > 1.0f || abs(mouseY) > 1.0f)
    {
        ndc_x = (2.0f * mouseX / (float)APP_VIRTUAL_WIDTH) - 1.0f;
        ndc_y = 1.0f - (2.0f * mouseY / (float)APP_VIRTUAL_HEIGHT);
    }

    float zoom = 15.0f;
    float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;
    float viewWidth = zoom * aspectRatio;
    float viewHeight = zoom;

    float viewX = ndc_x * (viewWidth / 2.0f);
    float viewY = ndc_y * (viewHeight / 2.0f);

    vec3d vUp = { 0.0f, 1.0f, 0.0f };
    mat4x4 matCamera = Matrix_PointAt(vCamera, vFocusPoint, vUp);

    vec3d rayStartView = { viewX, viewY, -10.0f };
    vec3d rayEndView = { viewX, viewY,  50.0f };

    vec3d rayStart = Matrix_MultiplyVector(matCamera, rayStartView);
    vec3d rayEnd = Matrix_MultiplyVector(matCamera, rayEndView);

    vec3d rayDir = Vector_Normalise(Vector_Sub(rayEnd, rayStart));

    if (abs(rayDir.y) < 0.001f) return { 0,0,0 };

    float t = (0.0f - rayStart.y) / rayDir.y;
    vec3d worldPos = Vector_Add(rayStart, Vector_Mul(rayDir, t));
    worldPos.y = 0.0f;

    return worldPos;
}

mesh CreateScaledWarrior(float scale) {
    mesh raw = ShapeBuilder::CreateWarrior();
    mesh finalMesh;
    ShapeBuilder::AddMesh(finalMesh, raw, { 0,0,0 }, { scale, scale, scale });
    return finalMesh;
}

mesh CreateEnemyWarrior(float scale) {
    mesh raw = ShapeBuilder::CreateEnemyWarrior();
    mesh finalMesh;
    ShapeBuilder::AddMesh(finalMesh, raw, { 0,0,0 }, { scale, scale, scale });
    return finalMesh;
}

//------------------------------------------------------------------------
// SPAWNING HELPERS
//------------------------------------------------------------------------

void SpawnEnemySquad(int count, vec3d position)
{
    Entity leader = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(leader, TransformComponent{ position });
    gCoordinator.AddComponent(leader, SquadComponent{ 1, position, 0, 0, 4.0f });

    float meleeWeight = 0.7f;
    float rangedWeight = 0.2f;
    float scale = 0.3f;

    for (int i = 0; i < count; i++)
    {
        Entity grunt = gCoordinator.CreateEntity();
        vec3d spawnPos = { position.x + (rand() % 10) / 10.0f, 0, position.z + (rand() % 10) / 10.0f };

        gCoordinator.AddComponent(grunt, TransformComponent{ spawnPos });
        float roll = static_cast <float>(rand()) / static_cast <float>(RAND_MAX);
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
        gCoordinator.AddComponent(grunt, ColliderComponent{ scale * 0.5f });
        gCoordinator.AddComponent(grunt, SquadMemberComponent{ leader, {0,0,0} });
        gCoordinator.AddComponent(grunt, AIComponent{ AIComponent::Type::Chaser });
    }

    squadSystem->RecalculateFormation(leader);
}

void SpawnPlayerUnit(vec3d position, UnitComponent::UnitType type, float scale = 0.3f) {
    Entity grunt = gCoordinator.CreateEntity();
    vec3d spawnPos = { position.x + (rand() % 10) / 10.0f, 0, position.z + (rand() % 10) / 10.0f };

    gCoordinator.AddComponent(grunt, TransformComponent{ spawnPos });
    gCoordinator.AddComponent(grunt, FactionComponent{ 0 });
    gCoordinator.AddComponent(grunt, ColliderComponent{ scale * 0.5f });

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

    gCoordinator.AddComponent(grunt, MeshComponent{ CreateScaledWarrior(scale) });
    gCoordinator.AddComponent(grunt, UnitComponent{ type, spawnPos, false, playerSpeed, false });
    gCoordinator.AddComponent(grunt, SquadMemberComponent{ playerUnit, {0,0,0} });
    gCoordinator.AddComponent(grunt, AIComponent{ AIComponent::Type::Wander });

    squadSystem->RecalculateFormation(playerUnit);
}

void SpawnPlayerSquad(int count, vec3d position)
{
    Entity leader = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(leader, TransformComponent{ position });
    gCoordinator.AddComponent(leader, SquadComponent{ 0, position, 0, 0, 10.0f });

    playerUnit = leader;

    for (int i = 0; i < count; i++)
    {
        SpawnPlayerUnit(position, UnitComponent::UnitType::Ranged);
    }
}

//------------------------------------------------------------------------
// INIT HELPERS
//------------------------------------------------------------------------

void RegisterComponents() {
    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<MeshComponent>();
    gCoordinator.RegisterComponent<UILabel>();
    gCoordinator.RegisterComponent<UIButton>();
    gCoordinator.RegisterComponent<UIProgressBar>();
    gCoordinator.RegisterComponent<StatComponent>();
    gCoordinator.RegisterComponent<BuilderComponent>();
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
    gCoordinator.RegisterComponent< OccupyingDepositComponent>();
}

void RegisterSystems() {
    // Render 3D
    render3D = gCoordinator.RegisterSystem<Render3DSystem>();
    Signature sig3D;
    sig3D.set(gCoordinator.GetComponentType<TransformComponent>());
    sig3D.set(gCoordinator.GetComponentType<MeshComponent>());
    gCoordinator.SetSystemSignature<Render3DSystem>(sig3D);

    // Render UI
    renderUI = gCoordinator.RegisterSystem<UIRenderSystem>();
    Signature sigUI;
    sigUI.set(gCoordinator.GetComponentType<UILabel>());
    gCoordinator.SetSystemSignature<UIRenderSystem>(sigUI);

    // Render Buttons
    renderButtonUI = gCoordinator.RegisterSystem<UIButtonSystem>();
    Signature sigBtton;
    sigBtton.set(gCoordinator.GetComponentType<UIButton>());
    gCoordinator.SetSystemSignature<UIButtonSystem>(sigBtton);

    // Render Progress Bars
    progressBarSystem = gCoordinator.RegisterSystem<UIProgressBarSystem>();
    Signature sigBar;
    sigBar.set(gCoordinator.GetComponentType<UIProgressBar>());
    gCoordinator.SetSystemSignature<UIProgressBarSystem>(sigBar);

    // Units
    unitSystem = gCoordinator.RegisterSystem<UnitSystem>();
    Signature sigUnit;
    sigUnit.set(gCoordinator.GetComponentType<TransformComponent>());
    sigUnit.set(gCoordinator.GetComponentType<UnitComponent>());
    gCoordinator.SetSystemSignature<UnitSystem>(sigUnit);

    // Collisions
    collisionSystem = gCoordinator.RegisterSystem<CollisionSystem>();
    Signature sigCol;
    sigCol.set(gCoordinator.GetComponentType<TransformComponent>());
    sigCol.set(gCoordinator.GetComponentType<ColliderComponent>());
    //sigCol.set(gCoordinator.GetComponentType<FactionComponent>());
    sigCol.set(gCoordinator.GetComponentType<StatComponent>());
	sigCol.set(gCoordinator.GetComponentType<ProjectileComponent>());
    gCoordinator.SetSystemSignature<CollisionSystem>(sigCol);

    // Squads
    squadSystem = gCoordinator.RegisterSystem<SquadSystem>();
    Signature sigSquad;
    sigSquad.set(gCoordinator.GetComponentType<TransformComponent>());
    gCoordinator.SetSystemSignature<SquadSystem>(sigSquad);

    // Player Control
    playerSystem = gCoordinator.RegisterSystem<PlayerControlSystem>();
    Signature sigPlayer;
    sigPlayer.set(gCoordinator.GetComponentType<UnitComponent>());
    gCoordinator.SetSystemSignature<PlayerControlSystem>(sigPlayer);

    // Animation
    animationSystem = gCoordinator.RegisterSystem<AnimationSystem>();
    Signature sigAnim;
    sigAnim.set(gCoordinator.GetComponentType<ArcAnimComponent>());
    gCoordinator.SetSystemSignature<AnimationSystem>(sigAnim);

    // Projectiles
    projectileSystem = gCoordinator.RegisterSystem<ProjectileSystem>();
    Signature sigProj;
    sigProj.set(gCoordinator.GetComponentType<ProjectileComponent>());
    sigProj.set(gCoordinator.GetComponentType<TransformComponent>());
    gCoordinator.SetSystemSignature<ProjectileSystem>(sigProj);

    // Resources
    resourceSystem = gCoordinator.RegisterSystem<ResourceSystem>();
    Signature sigRes;
    sigRes.set(gCoordinator.GetComponentType<TransformComponent>());
    sigRes.set(gCoordinator.GetComponentType<GoldDepositComponent>());
    gCoordinator.SetSystemSignature<ResourceSystem>(sigRes);

    // Production
    productionSystem = gCoordinator.RegisterSystem<ProductionSystem>();
    Signature sigProd;
    sigProd.set(gCoordinator.GetComponentType<FactoryComponent>());
    gCoordinator.SetSystemSignature<ProductionSystem>(sigProd);
}

void SetupWorld() {
    // -- Ground --
    Entity ground = gCoordinator.CreateEntity();
    mesh groundMesh = ShapeBuilder::CreatePlane(500.0f, 0.2f, 0.5f, 0.2f);
    gCoordinator.AddComponent(ground, TransformComponent{ {0, -0.1f, 0} });
    gCoordinator.AddComponent(ground, MeshComponent{ groundMesh });

    // -- Buildings --
    Entity factory = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(factory, TransformComponent{ {5, 0, 5} });
    gCoordinator.AddComponent(factory, MeshComponent{ ShapeBuilder::CreateFactoryUnit() });

    // -- Gold Chunks (Resource System) --
    for (int i = 0; i < 20; i++) {
        Entity goldChunk = gCoordinator.CreateEntity();
        float gx = (rand() % 80 - 40) * 1.0f;
        float gz = (rand() % 80 - 40) * 1.0f;

        gCoordinator.AddComponent(goldChunk, TransformComponent{ {gx, 0, gz} });
        gCoordinator.AddComponent(goldChunk, MeshComponent{ ShapeBuilder::CreateCube(1.0f, 0.8f, 0.0f, false) });
        gCoordinator.AddComponent(goldChunk, ColliderComponent{ 1.0f });
        gCoordinator.AddComponent(goldChunk, GoldDepositComponent{});
    }

    // -- Shrubbery / Trees (Decoration) --
    for (int i = 0; i < 50; ++i) {
        Entity tree = gCoordinator.CreateEntity();
        float x = (rand() % 100) - 50.0f;
        float z = (rand() % 100) - 50.0f;

        if (abs(x) < 10 && abs(z) < 10) continue; // Scale offset

        gCoordinator.AddComponent(tree, TransformComponent{ {x, 0, z} });
        gCoordinator.AddComponent(tree, MeshComponent{ ShapeBuilder::CreateTree() });
        gCoordinator.AddComponent(tree, ColliderComponent{ 0.5f });
    }

    // -- Units --
    SpawnEnemySquad(50.0f, { -10, 0, 5 });
    SpawnEnemySquad(35.0f, { -10, 0, -50 });
    SpawnEnemySquad(20.0f, { 15, 0, -15 });
    SpawnPlayerSquad(100, { 0, 0, 0 });
}

void SetupUI() {



    float cardWidth = 120.0f;
    float cardHeight = 80.0f;
    float cardSpacing = 20.0f;
    float centerX = APP_VIRTUAL_WIDTH / 2.0f;
    float bottomY = APP_VIRTUAL_HEIGHT - cardHeight - 20.0f;

    btnSpawnMelee = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(btnSpawnMelee, UIButton{
        centerX - (cardWidth * 1.5f + cardSpacing), bottomY, cardWidth, cardHeight, "Melee\n10g", 0.5f, 0.7f, 0.3f
        });

    btnSpawnRanged = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(btnSpawnRanged, UIButton{
        centerX - (cardWidth / 2.0f), bottomY, cardWidth, cardHeight, "Ranged\n20g", 0.3f, 0.5f, 0.8f
        });

    btnSpawnCatapult = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(btnSpawnCatapult, UIButton{
        centerX + (cardWidth / 2.0f + cardSpacing), bottomY, cardWidth, cardHeight, "Catapult\n50g", 0.8f, 0.3f, 0.3f
        });


    btnBuild = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(btnBuild, UIButton{ centerX - (cardWidth * 2.7f + cardSpacing), bottomY, cardWidth, cardHeight, "Build Factory", 0.2f, 0.2f, 0.8f
        });

    playerGold = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(playerGold, GoldComponent{ 0 });
    gCoordinator.AddComponent(playerGold, UILabel{ 30, APP_VIRTUAL_HEIGHT - 60, "Gold: 0", 1, 1, 1 });



    mouseCursor = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(mouseCursor, TransformComponent{ {0,0,0} });
}

void SetupCamera() {
    float zoom = 15.0f;
    float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;
    matProj = Engine3D::Matrix_MakeOrthographic(zoom * aspectRatio, zoom, -500.0f, 5000.0f);

    fYaw = 0.785398f;
    fTheta = 0.615472f;
    vFocusPoint = { 0, 0, 0 };

    mat4x4 matPitch = Matrix_MakeRotationX(fTheta);
    mat4x4 matYaw = Matrix_MakeRotationY(fYaw);
    mat4x4 matRot = Matrix_MultiplyMatrix(matPitch, matYaw);
    vec3d vOffset = { 0.0f, 0.0f, -20.0f };
    vOffset = Matrix_MultiplyVector(matRot, vOffset);
    vCamera = Vector_Add(vFocusPoint, vOffset);
}

//------------------------------------------------------------------------
// CORE FUNCTIONS
//------------------------------------------------------------------------

void Init()
{
    gCoordinator.Init();
    RegisterComponents();
    RegisterSystems();
    SetupWorld();
    SetupUI();
    SetupCamera();

    isMousePressed = false;
    isRightPressed = false;
}

void Update(const float deltaTime)
{
    // --- 1. Input & Calculations ---
    float screenCenterX = (float)APP_VIRTUAL_WIDTH / 2.0f;
    float screenCenterY = (float)APP_VIRTUAL_HEIGHT / 2.0f;
    vec3d worldCenter = GetIsoWorldCoordinates(screenCenterX, screenCenterY);

    float mouseX, mouseY;
    App::GetMousePos(mouseX, mouseY);
    float mouseYUI = APP_VIRTUAL_HEIGHT - mouseY;

    isMousePressed = App::IsMousePressed(GLUT_LEFT_BUTTON);
    bool isRightDown = App::IsMousePressed(GLUT_RIGHT_BUTTON);
    bool isRightClicked = isRightDown && !wasRightPressed;
    wasRightPressed = isRightDown;
    isRightPressed = isRightDown;

    // --- 2. System Updates ---
    unitSystem->Update(deltaTime);
    collisionSystem->Update(deltaTime);
    squadSystem->Update(deltaTime);
    animationSystem->Update(deltaTime);
    projectileSystem->Update(deltaTime);
    // resourceSystem->Update(deltaTime); // Commented out in source

    // Production Update (Gold Generation)
    productionSystem->Update(deltaTime / 1000.0f, playerGold);

    // --- 3. UI Logic ---
    auto& gold = gCoordinator.GetComponent<GoldComponent>(playerGold);
    auto& goldLabel = gCoordinator.GetComponent<UILabel>(playerGold);

    auto& btnMeleeUI = gCoordinator.GetComponent<UIButton>(btnSpawnMelee);
    auto& btnRangedUI = gCoordinator.GetComponent<UIButton>(btnSpawnRanged);
    auto& btnCatapultUI = gCoordinator.GetComponent<UIButton>(btnSpawnCatapult);
    btnMeleeUI.isDisabled = (gold.gold < 10);
    btnRangedUI.isDisabled = (gold.gold < 20);
    btnCatapultUI.isDisabled = (gold.gold < 50);

    // Handle Clicks
    Entity clickedID = renderButtonUI->UpdateInput(mouseX, mouseYUI, isMousePressed);

    // Simple helper to deduce gold and spawn
    auto TrySpawn = [&](int cost, UnitComponent::UnitType type) {
        if (gold.gold >= cost) {
            SpawnPlayerUnit(worldCenter, type);
            gold.gold -= cost;
            goldLabel.text = "Gold: " + std::to_string(gold.gold);
        }
        };

    if (clickedID == btnSpawnUnit)     TrySpawn(0, UnitComponent::UnitType::meleeGrunt);
    if (clickedID == btnSpawnMelee)    TrySpawn(10, UnitComponent::UnitType::meleeGrunt);
    if (clickedID == btnSpawnRanged)   TrySpawn(20, UnitComponent::UnitType::Ranged);
    if (clickedID == btnSpawnCatapult) TrySpawn(50, UnitComponent::UnitType::Catapult);

    if (clickedID == btnScore) {
        gold.gold += 10;
        goldLabel.text = "Gold: " + std::to_string(gold.gold);
    }

    if (clickedID == btnBuild) {
        // Only add components if not already in building mode
        if (!gCoordinator.HasComponent<BuilderComponent>(mouseCursor))
        {
            gCoordinator.AddComponent(mouseCursor, BuilderComponent{ 1, true });
            gCoordinator.AddComponent(mouseCursor, MeshComponent{ ShapeBuilder::CreateFactoryUnit() });
        }
    }

    // --- 4. Building Mode Logic ---
    if (gCoordinator.HasComponent<BuilderComponent>(mouseCursor))
    {
        vec3d worldPos = GetIsoWorldCoordinates(mouseX, mouseY);
        auto& trans = gCoordinator.GetComponent<TransformComponent>(mouseCursor);
        trans.Pos.x = worldPos.x;
        trans.Pos.z = worldPos.z;
        trans.Pos.y = 0.0f;

        // Check for valid placement (Gold Chunks)
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

        // Visual Feedback (Tint)
        auto& ghostMeshComp = gCoordinator.GetComponent<MeshComponent>(mouseCursor);
        ghostMeshComp.mesh = ShapeBuilder::CreateFactoryUnit(); // Reset mesh
        if (!canBuild) {
            ShapeBuilder::TintMesh(ghostMeshComp.mesh, 0.2f, 0.2f, 1.0f); // Blue tint if invalid
        }

        // Place Building
        if (isMousePressed && canBuild) {
            Entity newFactory = gCoordinator.CreateEntity();
            gCoordinator.AddComponent(newFactory, TransformComponent{ {worldPos.x, 0, worldPos.z} });
            gCoordinator.AddComponent(newFactory, MeshComponent{ ShapeBuilder::CreateFactoryUnit() });
            gCoordinator.AddComponent(newFactory, FactoryComponent{});
            gCoordinator.AddComponent(newFactory, FactionComponent{ 0 });
            gCoordinator.AddComponent(newFactory, StatComponent{ 1000, 1000, 0, 0 });
            gCoordinator.AddComponent(newFactory, ColliderComponent{ 1.0f });
 /*           gCoordinator.AddComponent(newFactory, UIProgressBar{
                worldPos.x, worldPos.z, 10, 5, 0.0f, 1.0f, 0.0f, 0
                });*/

            if (targetGoldChunk != -1) {
                auto& deposit = gCoordinator.GetComponent<GoldDepositComponent>(targetGoldChunk);
                deposit.occupied = true;
                deposit.linkedFactory = newFactory;
            }

            gCoordinator.RemoveComponent<BuilderComponent>(mouseCursor);
            gCoordinator.RemoveComponent<MeshComponent>(mouseCursor);
        }
    }
    else {
        // Only update player movement if not building
        playerSystem->Update(deltaTime);
    }

    // Squad Leader Target Logic
    if (worldCenter.x != 0.0f || worldCenter.z != 0.0f) {
        Entity playerLeader = playerUnit;
        if (playerLeader != -1) {
            auto& trans = gCoordinator.GetComponent<TransformComponent>(playerLeader);
            trans.Pos = worldCenter;
        }
    }

    // --- 5. Camera Movement ---
    float speed = playerSpeed * deltaTime / 1000.0f;
    if (App::IsKeyPressed(App::KEY_W)) vFocusPoint.z += speed;
    if (App::IsKeyPressed(App::KEY_S)) vFocusPoint.z -= speed;
    if (App::IsKeyPressed(App::KEY_A)) vFocusPoint.x -= speed;
    if (App::IsKeyPressed(App::KEY_D)) vFocusPoint.x += speed;

    float mapLimit = 500.0f;
    if (vFocusPoint.z > mapLimit) vFocusPoint.z = mapLimit;
    if (vFocusPoint.z < -mapLimit) vFocusPoint.z = -mapLimit;
    if (vFocusPoint.x > mapLimit) vFocusPoint.x = mapLimit;
    if (vFocusPoint.x < -mapLimit) vFocusPoint.x = -mapLimit;

    mat4x4 matPitch = Matrix_MakeRotationX(fTheta);
    mat4x4 matYaw = Matrix_MakeRotationY(fYaw);
    mat4x4 matRot = Matrix_MultiplyMatrix(matPitch, matYaw);
    vec3d vOffset = { 0.0f, 0.0f, -20.0f };
    vOffset = Matrix_MultiplyVector(matRot, vOffset);
    vCamera = Vector_Add(vFocusPoint, vOffset);
}

void Render()
{
    vec3d vUp = { 0.0f, 1.0f, 0.0f };
    mat4x4 matCamera = Matrix_PointAt(vCamera, vFocusPoint, vUp);
    mat4x4 matView = Matrix_QuickInverse(matCamera);

    render3D->Draw(matView, matProj, vCamera);
    //progressBarSystem->Draw(matView, matProj);
    renderUI->Draw();
    renderButtonUI->Draw();
}

void Shutdown()
{
    // Cleanup if necessary
}