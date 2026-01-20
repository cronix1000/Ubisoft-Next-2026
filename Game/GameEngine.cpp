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
#include "ParticleSystem.h"
#include "ParticleComponent.h"

using namespace Engine3D;

Coordinator gCoordinator;
Entity playerGold;
Entity mouseCursor;
Entity btnBuild;
Entity btnSpawnUnit;
Entity btnSpawnMelee;
Entity btnSpawnRanged;
Entity btnSpawnCatapult;
Entity notificationLabel;
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
std::shared_ptr<UIProgressBarSystem> progressBarSystem;
std::shared_ptr<ResourceSystem> resourceSystem;
std::shared_ptr<ProductionSystem> productionSystem;
std::shared_ptr<ParticleSystem> particleSystem;

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
bool hasMoved = false;
bool hasBoughtUnit = false;
bool hasPlacedFactory = false;
int initialSquadSize = 0;

float gameTimer = 300000.0f; // 5 minutes
bool gameOver = false;
Entity timerLabel;
Entity enemyCountLabel;

// Converts screen coordinates to world coordinates using raycasting
// Projects a ray from the camera through the mouse position and intersects it with the ground plane (y=0)
vec3d GetIsoWorldCoordinates(float mouseX, float mouseY)
{
    // Convert screen coordinates to normalized device coordinates (-1 to 1)
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

    // Prevent division by zero if ray is parallel to ground
    if (abs(rayDir.y) < 0.001f) return { 0,0,0 };

    // Calculate intersection parameter t where ray meets ground plane (y=0)
    float t = (0.0f - rayStart.y) / rayDir.y;
    vec3d worldPos = Vector_Add(rayStart, Vector_Mul(rayDir, t));
    worldPos.y = 0.0f;

    return worldPos;
}

mesh CreateScaledWarrior(float scale, UnitComponent::UnitType type) {
    mesh raw;
    mesh finalMesh;
	switch (type)
	{
	case UnitComponent::UnitType::meleeGrunt:
	raw = ShapeBuilder::CreateWarrior();
		break;
	case UnitComponent::UnitType::Ranged:
		raw = ShapeBuilder::CreateRangedWarrior();
		break;
	case UnitComponent::UnitType::Catapult:
		raw = ShapeBuilder::CreateCatapult();
		break;	
	default:
	raw = ShapeBuilder::CreateWarrior(); 
		break;
	}
    ShapeBuilder::AddMesh(finalMesh, raw, { 0,0,0 }, { scale, scale, scale });
    return finalMesh;
}

mesh CreateEnemyWarrior(float scale, UnitComponent::UnitType type) {
    mesh raw;
	switch (type)
	{
	case UnitComponent::UnitType::meleeGrunt:
		raw = ShapeBuilder::CreateEnemyWarrior();
		break;
	case UnitComponent::UnitType::Ranged:
		raw = ShapeBuilder::CreateEnemyRangedWarrior();
		break;
	case UnitComponent::UnitType::Catapult:
		raw = ShapeBuilder::CreateCatapult();
		break;
	default:
	raw = ShapeBuilder::CreateEnemyWarrior();
		break;
	}
    mesh finalMesh;
    ShapeBuilder::AddMesh(finalMesh, raw, { 0,0,0 }, { scale, scale, scale });
    return finalMesh;
}

void SpawnEnemySquad(int count, vec3d position)
{
    Entity leader = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(leader, TransformComponent{ position });
    gCoordinator.AddComponent(leader, SquadComponent{ 1, position, 0, 0, 4.0f });

    // Unit composition: 60% melee, 30% ranged, 10% catapult for balanced enemy squads
    float meleeWeight = 0.6f;
    float rangedWeight = 0.3f;
    float scale = 0.1f;

    for (int i = 0; i < count; i++)
    {
        Entity grunt = gCoordinator.CreateEntity();
        // Randomize spawn position within 1x1 area to prevent perfect overlap
        vec3d spawnPos = { position.x + (rand() % 10) / 10.0f, 0, position.z + (rand() % 10) / 10.0f };

        gCoordinator.AddComponent(grunt, TransformComponent{ spawnPos });
        // Weighted random selection for unit type diversity
        float roll = static_cast <float>(rand()) / static_cast <float>(RAND_MAX);
        UnitComponent::UnitType unitType;

        if (roll < meleeWeight) {
            unitType = UnitComponent::UnitType::meleeGrunt;
            gCoordinator.AddComponent(grunt, StatComponent{ 50, 50, 8, 1 });
            scale = 0.3f;
        }
        else if (roll < meleeWeight + rangedWeight) {
            unitType = UnitComponent::UnitType::Ranged;
            gCoordinator.AddComponent(grunt, StatComponent{ 40, 40, 0, 1 });
            scale = 0.25f;
        }
        else {
            unitType = UnitComponent::UnitType::Catapult;
            gCoordinator.AddComponent(grunt, StatComponent{ 60, 60, 0, 1 });
            scale = 0.4f;
        }

        gCoordinator.AddComponent(grunt, UnitComponent{ unitType, spawnPos, false, 5.0f, false });
        gCoordinator.AddComponent(grunt, MeshComponent{ CreateEnemyWarrior(scale, unitType) });
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
        gCoordinator.AddComponent(grunt, StatComponent{ 150, 150, 25, 0 });
        scale = 0.3f;
        break;
    case UnitComponent::UnitType::Ranged:
        gCoordinator.AddComponent(grunt, StatComponent{ 120, 120, 0, 0 });
        scale = 0.5f;
        break;
    case UnitComponent::UnitType::Catapult:
        gCoordinator.AddComponent(grunt, StatComponent{ 200, 200, 0, 0 });
        scale = 0.8f;
        break;
    }

    gCoordinator.AddComponent(grunt, MeshComponent{ CreateScaledWarrior(scale, type) });
    gCoordinator.AddComponent(grunt, UnitComponent{ type, spawnPos, false, playerSpeed, false });
    gCoordinator.AddComponent(grunt, SquadMemberComponent{ playerUnit, {0,0,0} });
    gCoordinator.AddComponent(grunt, AIComponent{ AIComponent::Type::Wander });

    squadSystem->RecalculateFormation(playerUnit);
}

void SpawnPlayerSquad(int meleeCount, int rangedCount, vec3d position)
{
    Entity leader = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(leader, TransformComponent{ position });
    gCoordinator.AddComponent(leader, SquadComponent{ 0, position, 0, 0, 10.0f });

    playerUnit = leader;

    for (int i = 0; i < meleeCount; i++)
    {
        SpawnPlayerUnit(position, UnitComponent::UnitType::meleeGrunt);
    }
    
    for (int i = 0; i < rangedCount; i++)
    {
        SpawnPlayerUnit(position, UnitComponent::UnitType::Ranged);
    }
}

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
	gCoordinator.RegisterComponent<ParticleComponent>();
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

    // Collisions (units AND projectiles need collisions)
    collisionSystem = gCoordinator.RegisterSystem<CollisionSystem>();
    Signature sigCol;
    sigCol.set(gCoordinator.GetComponentType<TransformComponent>());
    sigCol.set(gCoordinator.GetComponentType<ColliderComponent>());
    sigCol.set(gCoordinator.GetComponentType<StatComponent>());
    // Don't require ProjectileComponent - units need collisions too!
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

	// Particles
	particleSystem = gCoordinator.RegisterSystem<ParticleSystem>();
    Signature sigPart;
    sigPart.set(gCoordinator.GetComponentType<ParticleComponent>());
    sigPart.set(gCoordinator.GetComponentType<TransformComponent>());
    sigPart.set(gCoordinator.GetComponentType<MeshComponent>());
    gCoordinator.SetSystemSignature<ParticleSystem>(sigPart);

}

void SetupWorld() {
    Entity ground = gCoordinator.CreateEntity();
    mesh groundMesh = ShapeBuilder::CreatePlane(100.0f, 0.2f, 0.5f, 0.2f);
    gCoordinator.AddComponent(ground, TransformComponent{ {0, -0.1f, 0} });
    
    MeshComponent groundMeshComp;
    groundMeshComp.mesh = groundMesh;
    groundMeshComp.isImportant = true;
    gCoordinator.AddComponent(ground, groundMeshComp);

    for (int i = 0; i < 20; i++) {
        Entity goldChunk = gCoordinator.CreateEntity();
        float gx = (rand() % 80 - 40) * 1.0f;
        float gz = (rand() % 80 - 40) * 1.0f;

        gCoordinator.AddComponent(goldChunk, TransformComponent{ {gx, 0, gz} });
        gCoordinator.AddComponent(goldChunk, MeshComponent{ ShapeBuilder::CreateCube(1.0f, 0.8f, 0.0f, false) });
        gCoordinator.AddComponent(goldChunk, ColliderComponent{ 1.0f });
        gCoordinator.AddComponent(goldChunk, GoldDepositComponent{});
    }

    for (int i = 0; i < 50; ++i) {
        Entity tree = gCoordinator.CreateEntity();
        float x = (rand() % 100) - 50.0f;
        float z = (rand() % 100) - 50.0f;

        if (abs(x) < 10 && abs(z) < 10) continue;

        gCoordinator.AddComponent(tree, TransformComponent{ {x, 0, z} });
        gCoordinator.AddComponent(tree, MeshComponent{ ShapeBuilder::CreateTree() });
        gCoordinator.AddComponent(tree, ColliderComponent{ 0.5f });
    }
    SpawnEnemySquad(50.0f, { -10, 0, -50 });
    SpawnEnemySquad(40.0f, { 15, 0, -15 });
    SpawnEnemySquad(50.0f, { 40, 0, 20 });
    SpawnEnemySquad(45.0f, { -35, 0, -30 });
    SpawnEnemySquad(50.0f, { -50, 0, -30 });
    SpawnEnemySquad(35.0f, { 50, 0, -40 });
    SpawnEnemySquad(40.0f, { -50, 0, 40 });
    SpawnEnemySquad(45.0f, { -20, 0, 50 });
    SpawnEnemySquad(40.0f, { 30, 0, -50 });
    SpawnEnemySquad(35.0f, { -40, 0, -50 });
    SpawnEnemySquad(30.0f, { 60, 0, 10 });
    SpawnEnemySquad(30.0f, { -60, 0, -10 });

    SpawnPlayerSquad(50, 40, { 0, 0, 0 });
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
    gCoordinator.AddComponent(btnBuild, UIButton{ centerX - (cardWidth * 2.8f + cardSpacing), bottomY, cardWidth + 5, cardHeight, "Factory 20g", 0.2f, 0.2f, 0.8f
        });

    playerGold = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(playerGold, GoldComponent{ 50 });
    gCoordinator.AddComponent(playerGold, UILabel{ 30, APP_VIRTUAL_HEIGHT - 60, "Gold: 50", 1, 1, 1 });


    notificationLabel = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(notificationLabel, UILabel{
        APP_VIRTUAL_WIDTH / 2 - 200,
        50,
        "Press WASD to move the camera",
        1.0f, 1.0f, 0.0f
        });

    timerLabel = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(timerLabel, UILabel{
        APP_VIRTUAL_WIDTH - 150,
        APP_VIRTUAL_HEIGHT - 60,
        "Time: 5:00",
        1.0f, 1.0f, 1.0f
        });

    enemyCountLabel = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(enemyCountLabel, UILabel{
        APP_VIRTUAL_WIDTH - 150,
        APP_VIRTUAL_HEIGHT - 90,
        "Enemies: 0",
        1.0f, 0.3f, 0.3f
        });

    mouseCursor = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(mouseCursor, TransformComponent{ {0,0,0} });
}

void SetupCamera() {
    float zoom = 20.0f;
    float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;
    // Orthographic projection prevents perspective distortion for isometric view
    matProj = Engine3D::Matrix_MakeOrthographic(zoom * aspectRatio, zoom, -500.0f, 5000.0f);

    // Camera angles: 45° yaw (0.785398 rad) and ~35° pitch (0.615472 rad) for isometric perspective
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

void Init()
{
    gCoordinator.Init();

    RegisterComponents();
    RegisterSystems();
    SetupWorld();
    SetupUI();
    SetupCamera();

	collisionSystem->Init();
    render3D->Init();

    isMousePressed = false;
    isRightPressed = false;
}

void Update(const float deltaTime)
{
    static float accumulatedTime = 0.0f;
    static int frameCount = 0;
    accumulatedTime += deltaTime;
    frameCount++;
    
    // Performance optimization: Run expensive AI/collision systems every 5th frame (~12Hz)
    // Movement/animation systems run every frame for smooth visuals (60Hz)
    bool runSlowSystems = (frameCount % 5 == 0);
    
    // Player squad leader follows screen center for camera-relative control
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

    unitSystem->Update(deltaTime);
    animationSystem->Update(deltaTime);
    particleSystem->Update(deltaTime);
    projectileSystem->Update(deltaTime);
    
    // Multiply deltaTime by 5 to compensate for running 1/5th as often
    if (runSlowSystems) {
        collisionSystem->Update(deltaTime * 5.0f);
        squadSystem->Update(deltaTime * 5.0f);
    }

    auto& gold = gCoordinator.GetComponent<GoldComponent>(playerGold);
    auto& goldLabel = gCoordinator.GetComponent<UILabel>(playerGold);

    if (runSlowSystems) {
        // Factories generate gold automatically when built near gold deposits
        productionSystem->Update(deltaTime / 1000.0f * 5.0f, playerGold);
        
        // Passive income: +1 gold every 2 seconds as base resource generation
        static float passiveGoldTimer = 0.0f;
        passiveGoldTimer += deltaTime * 5.0f;
        if (passiveGoldTimer >= 2000.0f) {
            gold.gold += 1;
            goldLabel.text = "Gold: " + std::to_string(gold.gold);
            passiveGoldTimer = 0.0f;
        }
    }

    auto& btnMeleeUI = gCoordinator.GetComponent<UIButton>(btnSpawnMelee);
    auto& btnRangedUI = gCoordinator.GetComponent<UIButton>(btnSpawnRanged);
    auto& btnCatapultUI = gCoordinator.GetComponent<UIButton>(btnSpawnCatapult);
    
    btnMeleeUI.isDisabled = (gold.gold < 10);
    btnRangedUI.isDisabled = (gold.gold < 20);
    btnCatapultUI.isDisabled = (gold.gold < 50);

    Entity clickedID = renderButtonUI->UpdateInput(mouseX, mouseYUI, isMousePressed);

    auto TrySpawn = [&](int cost, UnitComponent::UnitType type) {
        if (gold.gold >= cost) {
            SpawnPlayerUnit(worldCenter, type);
            gold.gold -= cost;
            goldLabel.text = "Gold: " + std::to_string(gold.gold);
        }
        };

    if (clickedID == btnSpawnMelee)    TrySpawn(10, UnitComponent::UnitType::meleeGrunt);
    if (clickedID == btnSpawnRanged)   TrySpawn(20, UnitComponent::UnitType::Ranged);
    if (clickedID == btnSpawnCatapult) TrySpawn(50, UnitComponent::UnitType::Catapult);

    if (clickedID == btnBuild && gold.gold >= 20) {
        if (!gCoordinator.HasComponent<BuilderComponent>(mouseCursor))
        {
            gCoordinator.AddComponent(mouseCursor, BuilderComponent{ 1, true });
            gCoordinator.AddComponent(mouseCursor, MeshComponent{ ShapeBuilder::CreateFactoryUnit() });
        }
    }

    if (gCoordinator.HasComponent<BuilderComponent>(mouseCursor))
    {
        vec3d worldPos = GetIsoWorldCoordinates(mouseX, mouseY);
        auto& trans = gCoordinator.GetComponent<TransformComponent>(mouseCursor);
        trans.Pos.x = worldPos.x;
        trans.Pos.z = worldPos.z;
        trans.Pos.y = 0.0f;

        // Factory must be placed within 2 units of an unoccupied gold deposit
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

        auto& ghostMeshComp = gCoordinator.GetComponent<MeshComponent>(mouseCursor);
        ghostMeshComp.mesh = ShapeBuilder::CreateFactoryUnit();
        if (!canBuild) {
            ShapeBuilder::TintMesh(ghostMeshComp.mesh, 0.2f, 0.2f, 1.0f);
        }

        if (isMousePressed && canBuild) {
            auto& gold = gCoordinator.GetComponent<GoldComponent>(playerGold);
            auto& goldLabel = gCoordinator.GetComponent<UILabel>(playerGold);
            
            if (gold.gold >= 20) {
                gold.gold -= 20;
                goldLabel.text = "Gold: " + std::to_string(gold.gold);
                
                Entity newFactory = gCoordinator.CreateEntity();
                gCoordinator.AddComponent(newFactory, TransformComponent{ {worldPos.x, 0, worldPos.z} });
                gCoordinator.AddComponent(newFactory, MeshComponent{ ShapeBuilder::CreateFactoryUnit() });
                gCoordinator.AddComponent(newFactory, FactoryComponent{});
                gCoordinator.AddComponent(newFactory, FactionComponent{ 0 });
                gCoordinator.AddComponent(newFactory, StatComponent{ 2000, 1000, 0, 0 });
                gCoordinator.AddComponent(newFactory, ColliderComponent{ 1.0f });

                // Bidirectional link: deposit->factory and factory->deposit
                // Ensures deposit is freed when factory is destroyed
                if (targetGoldChunk != -1) {
                    auto& deposit = gCoordinator.GetComponent<GoldDepositComponent>(targetGoldChunk);
                    deposit.occupied = true;
                    deposit.linkedFactory = newFactory;
                    gCoordinator.AddComponent(newFactory, OccupyingDepositComponent{ targetGoldChunk });
                }

                gCoordinator.RemoveComponent<BuilderComponent>(mouseCursor);
                gCoordinator.RemoveComponent<MeshComponent>(mouseCursor);
            }
        }
        
        if (isRightClicked) {
            gCoordinator.RemoveComponent<BuilderComponent>(mouseCursor);
            gCoordinator.RemoveComponent<MeshComponent>(mouseCursor);
        }
    }
    else {
        playerSystem->Update(deltaTime);

    float speed = playerSpeed * deltaTime / 1000.0f;
    if (App::IsKeyPressed(App::KEY_W)) vFocusPoint.z += speed;
    if (App::IsKeyPressed(App::KEY_S)) vFocusPoint.z -= speed;
    if (App::IsKeyPressed(App::KEY_A)) vFocusPoint.x -= speed;
    if (App::IsKeyPressed(App::KEY_D)) vFocusPoint.x += speed;


    if (vFocusPoint.z > MAP_LIMIT) vFocusPoint.z = MAP_LIMIT;
    if (vFocusPoint.z < -MAP_LIMIT) vFocusPoint.z = -MAP_LIMIT;
    if (vFocusPoint.x > MAP_LIMIT) vFocusPoint.x = MAP_LIMIT;
    if (vFocusPoint.x < -MAP_LIMIT) vFocusPoint.x = -MAP_LIMIT;
    }

    auto& note = gCoordinator.GetComponent<UILabel>(notificationLabel);
    
    if (!gameOver) {
        gameTimer -= deltaTime;
        
        int enemyCount = 0;
        int playerCount = 0;
        for (auto const& entity : squadSystem->mEntities) {
            if (gCoordinator.HasComponent<FactionComponent>(entity)) {
                auto& faction = gCoordinator.GetComponent<FactionComponent>(entity);
                if (faction.teamId == 1) {
                    enemyCount++;
                }
                else if (faction.teamId == 0) {
                    playerCount++;
                }
            }
        }
        
        auto& enemyText = gCoordinator.GetComponent<UILabel>(enemyCountLabel);
        enemyText.text = "Enemies: " + std::to_string(enemyCount);
        
        if (playerCount == 0) {
            gameOver = true;
            note.text = "DEFEAT! All your units are destroyed!";
            note.r = 1.0f; note.g = 0.0f; note.b = 0.0f;
        }
        
        if (enemyCount == 0) {
            gameOver = true;
            note.text = "VICTORY! All enemies defeated!";
            note.r = 0.0f; note.g = 1.0f; note.b = 0.0f;
        }
        
        auto& timerText = gCoordinator.GetComponent<UILabel>(timerLabel);
        int totalSeconds = static_cast<int>(gameTimer / 1000.0f);
        if (totalSeconds < 0) totalSeconds = 0;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        timerText.text = "Time: " + std::to_string(minutes) + ":" + 
                         (seconds < 10 ? "0" : "") + std::to_string(seconds);
        
        // Timer color: Red < 60s, Orange < 120s, White otherwise for urgency indication
        if (totalSeconds < 60) {
            timerText.r = 1.0f; timerText.g = 0.3f; timerText.b = 0.3f;
        } else if (totalSeconds < 120) {
            timerText.r = 1.0f; timerText.g = 0.7f; timerText.b = 0.0f;
        } else {
            timerText.r = 1.0f; timerText.g = 1.0f; timerText.b = 1.0f;
        }
        
        if (gameTimer <= 0.0f || playerCount <= 0) {
            gameOver = true;
            note.text = "TIME'S UP! GAME OVER!";
            note.r = 1.0f; note.g = 0.0f; note.b = 0.0f;
        }
    }
    
    if (!gameOver) {
    if (!hasMoved) {
        if (App::IsKeyPressed(App::KEY_W) || App::IsKeyPressed(App::KEY_A) ||
            App::IsKeyPressed(App::KEY_S) || App::IsKeyPressed(App::KEY_D)) {
            hasMoved = true;
            note.text = "Good! Now spawn a unit (click green Melee button)";
        }
    }
    else if (!hasBoughtUnit) {
        // Track squad size increase to detect unit purchase for tutorial progression
        int currentSquadSize = 0;
        for (auto const& entity : squadSystem->mEntities) {
            if (gCoordinator.HasComponent<SquadMemberComponent>(entity)) {
                auto& member = gCoordinator.GetComponent<SquadMemberComponent>(entity);
                if (member.squadId == playerUnit) currentSquadSize++;
            }
        }
        
        if (initialSquadSize == 0) {
            initialSquadSize = currentSquadSize;
        } else if (currentSquadSize > initialSquadSize) {
            hasBoughtUnit = true;
            note.x = 30,
                note.y = 50,
            note.text = "Great! Press Factory Button and click near gold to build a factory | Press Right Click to cancel";
        }
    }
    else if (!hasPlacedFactory) {
        if (productionSystem->mEntities.size() > 0) {
            hasPlacedFactory = true;
            note.x = APP_VIRTUAL_WIDTH / 2 - 200,
            note.text = "Perfect! Kill all enemies to win!";
        }
    }
    else {
        static float completionTimer = 0.0f;
        completionTimer += deltaTime;
        if (completionTimer > 5000.0f) {
            note.text = "";
        }
    }
    }
    // Update player squad leader position to screen center for camera-following behavior
    // Squad members automatically form around leader via SquadSystem
    if (worldCenter.x != 0.0f || worldCenter.z != 0.0f) {
        Entity playerLeader = playerUnit;
        if (playerLeader != -1) {
            auto& trans = gCoordinator.GetComponent<TransformComponent>(playerLeader);
            trans.Pos = worldCenter;
        }
    }

    mat4x4 matPitch = Matrix_MakeRotationX(fTheta);
    mat4x4 matYaw = Matrix_MakeRotationY(fYaw);
    mat4x4 matRot = Matrix_MultiplyMatrix(matPitch, matYaw);
    vec3d vOffset = { 0.0f, 0.0f, -20.0f };
    vOffset = Matrix_MultiplyVector(matRot, vOffset);
    vCamera = Vector_Add(vFocusPoint, vOffset);

    if (gameOver) {
        App::PlayAudio("./data/game_over.mp3");
        
    }
}

void Render()
{
    vec3d vUp = { 0.0f, 1.0f, 0.0f };
    mat4x4 matCamera = Matrix_PointAt(vCamera, vFocusPoint, vUp);
    mat4x4 matView = Matrix_QuickInverse(matCamera);

    render3D->Draw(matView, matProj, vCamera);
    renderUI->Draw();
    renderButtonUI->Draw();
}

void Shutdown()
{
}