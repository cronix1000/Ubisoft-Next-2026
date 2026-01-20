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
Entity notificationLabel;
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
std::shared_ptr<ParticleSystem> particleSystem;

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

// Tutorial tracking
bool hasMoved = false;
bool hasBoughtUnit = false;
bool hasPlacedFactory = false;
int initialSquadSize = 0;

// Game timer (5 minutes = 300,000 milliseconds)
float gameTimer = 300000.0f;
bool gameOver = false;
Entity timerLabel;
Entity enemyCountLabel;
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

mesh CreateScaledWarrior(float scale, UnitComponent::UnitType type) {
    mesh raw;
    mesh finalMesh;
	switch (type)
	{
	case UnitComponent::UnitType::meleeGrunt:
	raw = ShapeBuilder::CreateWarrior(); // Green/Hunter tunic
		break;
	case UnitComponent::UnitType::Ranged:
		raw = ShapeBuilder::CreateRangedWarrior(); // Green/Hunter tunic
		break;
	case UnitComponent::UnitType::Catapult:
		raw = ShapeBuilder::CreateCatapult(); // Brown catapult
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
		
		raw = ShapeBuilder::CreateEnemyWarrior(); // Red armor
		break;
	case UnitComponent::UnitType::Ranged:
		raw = ShapeBuilder::CreateEnemyRangedWarrior(); // Red/Hunter tunic
		break;
	case UnitComponent::UnitType::Catapult:
		
		raw = ShapeBuilder::CreateCatapult(); // Brown catapult
		break;
	default:
	raw = ShapeBuilder::CreateEnemyWarrior();
		break;
	}
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
    // -- Ground --
    Entity ground = gCoordinator.CreateEntity();
    // Ground plane is 1400x1400 (from -700 to +700) - larger than playable area to prevent edge issues
    mesh groundMesh = ShapeBuilder::CreatePlane(200.0f, 0.2f, 0.5f, 0.2f);
    gCoordinator.AddComponent(ground, TransformComponent{ {0, -0.1f, 0} });
    
    MeshComponent groundMeshComp;
    groundMeshComp.mesh = groundMesh;
    groundMeshComp.isImportant = true; 
    gCoordinator.AddComponent(ground, groundMeshComp);

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
    // SpawnEnemySquad(50.0f, { -10, 0, 5 });
     SpawnEnemySquad(80.0f, { -10, 0, -50 });
    SpawnEnemySquad(60.0f, { 15, 0, -15 });
    SpawnEnemySquad(80.0f, { 40, 0, 20 });
    SpawnEnemySquad(80.0f, { -35, 0, -30 });
    SpawnEnemySquad(40.0f, { 50, 0, -40 });
    SpawnEnemySquad(30.0f, { -50, 0, 40 });

    SpawnPlayerSquad(40, 30, { 0, 0, 0 });
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


	// INIT SYSTEMS
	collisionSystem->Init();
    render3D->Init();

    isMousePressed = false;
    isRightPressed = false;
}

void Update(const float deltaTime)
{
    // Update throttling - not all systems need to run every frame
    static float accumulatedTime = 0.0f;
    static int frameCount = 0;
    accumulatedTime += deltaTime;
    frameCount++;
    
    bool runSlowSystems = (frameCount % 5 == 0); // Run every 5th frame (~12Hz at 60fps) - better debug performance
    
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
    // Run movement/rendering systems every frame for smooth gameplay
    unitSystem->Update(deltaTime);
    animationSystem->Update(deltaTime);
    particleSystem->Update(deltaTime);
    projectileSystem->Update(deltaTime);
    
    // Throttle expensive AI/collision systems to reduce lag
    if (runSlowSystems) {
        collisionSystem->Update(deltaTime * 5.0f);
        squadSystem->Update(deltaTime * 5.0f);
    }
    // resourceSystem->Update(deltaTime); // Commented out in source

    auto& gold = gCoordinator.GetComponent<GoldComponent>(playerGold);
    auto& goldLabel = gCoordinator.GetComponent<UILabel>(playerGold);

    // Production Update (Gold Generation) - can be slower
    if (runSlowSystems) {
        productionSystem->Update(deltaTime / 1000.0f * 5.0f, playerGold);
        
        static float passiveGoldTimer = 0.0f;
        passiveGoldTimer += deltaTime * 5.0f;
        if (passiveGoldTimer >= 2000.0f) {
            gold.gold += 1;
            // update label
            goldLabel.text = "Gold: " + std::to_string(gold.gold);
            passiveGoldTimer = 0.0f;
        }
    }

    // --- 3. UI Logic ---


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

    if (clickedID == btnSpawnMelee)    TrySpawn(10, UnitComponent::UnitType::meleeGrunt);
    if (clickedID == btnSpawnRanged)   TrySpawn(20, UnitComponent::UnitType::Ranged);
    if (clickedID == btnSpawnCatapult) TrySpawn(50, UnitComponent::UnitType::Catapult);

    if (clickedID == btnBuild && gold.gold >= 20) {
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
            // Deduct gold for factory (cost is 20)
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
                gCoordinator.AddComponent(newFactory, StatComponent{ 1000, 1000, 0, 0 });
                gCoordinator.AddComponent(newFactory, ColliderComponent{ 1.0f });
     /*           gCoordinator.AddComponent(newFactory, UIProgressBar{
                    worldPos.x, worldPos.z, 10, 5, 0.0f, 1.0f, 0.0f, 0
                    });*/

                if (targetGoldChunk != -1) {
                    auto& deposit = gCoordinator.GetComponent<GoldDepositComponent>(targetGoldChunk);
                    deposit.occupied = true;
                    deposit.linkedFactory = newFactory;
                    
                    // Add the link component to the factory so collision system can free the deposit
                    gCoordinator.AddComponent(newFactory, OccupyingDepositComponent{ targetGoldChunk });
                }

                gCoordinator.RemoveComponent<BuilderComponent>(mouseCursor);
                gCoordinator.RemoveComponent<MeshComponent>(mouseCursor);
            }
        }
        
        // Cancel building mode (right click)
        if (isRightClicked) {
            gCoordinator.RemoveComponent<BuilderComponent>(mouseCursor);
            gCoordinator.RemoveComponent<MeshComponent>(mouseCursor);
        }
    }
    else {
        // Only update player movement if not building
        playerSystem->Update(deltaTime);

		    // --- 5. Camera Movement ---
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

    // --- TUTORIAL LOGIC ---
    auto& note = gCoordinator.GetComponent<UILabel>(notificationLabel);
    
    if (!gameOver) {
        gameTimer -= deltaTime;
        
        // Count enemies (faction team 1)
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
        
        // Update enemy count display
        auto& enemyText = gCoordinator.GetComponent<UILabel>(enemyCountLabel);
        enemyText.text = "Enemies: " + std::to_string(enemyCount);
        
        // Check lose condition - no player units left
        if (playerCount == 0) {
            gameOver = true;
            note.text = "DEFEAT! All your units are destroyed!";
            note.r = 1.0f; note.g = 0.0f; note.b = 0.0f;
        }
        
        // Check win condition
        if (enemyCount == 0) {
            gameOver = true;
            note.text = "VICTORY! All enemies defeated!";
            note.r = 0.0f; note.g = 1.0f; note.b = 0.0f;
        }
        
        // Update timer display
        auto& timerText = gCoordinator.GetComponent<UILabel>(timerLabel);
        int totalSeconds = static_cast<int>(gameTimer / 1000.0f);
        if (totalSeconds < 0) totalSeconds = 0;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        timerText.text = "Time: " + std::to_string(minutes) + ":" + 
                         (seconds < 10 ? "0" : "") + std::to_string(seconds);
        
        // Change color when time is running out
        if (totalSeconds < 60) {
            timerText.r = 1.0f; timerText.g = 0.3f; timerText.b = 0.3f; // Red
        } else if (totalSeconds < 120) {
            timerText.r = 1.0f; timerText.g = 0.7f; timerText.b = 0.0f; // Orange
        } else {
            timerText.r = 1.0f; timerText.g = 1.0f; timerText.b = 1.0f; // White
        }
        
        // Game over if time runs out
        if (gameTimer <= 0.0f) {
            gameOver = true;
            note.text = "TIME'S UP! GAME OVER!";
            note.r = 1.0f; note.g = 0.0f; note.b = 0.0f;
        }
    }
    
    // Skip tutorial updates if game is over
    if (!gameOver) {
    // Step 1: Wait for camera movement
    if (!hasMoved) {
        if (App::IsKeyPressed(App::KEY_W) || App::IsKeyPressed(App::KEY_A) ||
            App::IsKeyPressed(App::KEY_S) || App::IsKeyPressed(App::KEY_D)) {
            hasMoved = true;
            note.text = "Good! Now spawn a unit (click green Melee button)";
        }
    }
    // Step 2: Wait for unit purchase
    else if (!hasBoughtUnit) {
        // Check if squad size increased
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
            note.text = "Great! Press Factory Button and click near gold to build a factory | Press Right Click to cancel";
        }
    }
    // Step 3: Wait for factory placement
    else if (!hasPlacedFactory) {
        if (productionSystem->mEntities.size() > 0) {
            hasPlacedFactory = true;
            note.text = "Perfect! Kill all enemies to win!";
        }
    }
    // Tutorial complete - clear message after a few seconds
    else {
        static float completionTimer = 0.0f;
        completionTimer += deltaTime;
        if (completionTimer > 5000.0f) {
            note.text = "";
        }
    }
    } // End game over check

    // Squad Leader Target Logic
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