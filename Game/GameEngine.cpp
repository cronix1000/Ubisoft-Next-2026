
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

using namespace Engine3D;

Coordinator gCoordinator;
Entity playerStats; // Holds our Score
Entity mouseCursor; // Holds our "Ghost" builder state
Entity btnScore;
Entity btnBuild;

std::shared_ptr<Render3DSystem> render3D;
std::shared_ptr<UIRenderSystem> renderUI;
std::shared_ptr<UIButtonSystem> renderButtonUI;
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

vec3d GetIsoWorldCoordinates(float mouseX, float mouseY)
{
    // 1. Convert Mouse to Normalized Device Coordinates (NDC)
    // Screen is 0..Width, 0..Height. NDC is -1..1
    float ndc_x = (2.0f * mouseX / (float)APP_VIRTUAL_WIDTH) - 1.0f;
    float ndc_y = 1.0f - (2.0f * mouseY / (float)APP_VIRTUAL_HEIGHT); // Flip Y

    // 2. Create the View-Projection Matrix
    // We need to reconstruct the camera matrix used in Render()
    vec3d vUp = { 0.0f, 1.0f, 0.0f };
    mat4x4 matCamera = Matrix_PointAt(vCamera, vFocusPoint, vUp);
    mat4x4 matView = Matrix_QuickInverse(matCamera);
    mat4x4 matViewProj = Matrix_MultiplyMatrix(matView, matProj);
    mat4x4 matInvViewProj = Matrix_QuickInverse(matViewProj);

    // 3. Unproject 2 points (Near Plane and Far Plane) to get a Ray
    // Z = -1.0 (Near), Z = 1.0 (Far)
    vec3d rayStart = { ndc_x, ndc_y, -1.0f, 1.0f };
    vec3d rayEnd = { ndc_x, ndc_y,  1.0f, 1.0f };

    // Transform from Clip Space back to World Space
    rayStart = Matrix_MultiplyVector(matInvViewProj, rayStart);
    rayStart = Vector_Div(rayStart, rayStart.w);

    rayEnd = Matrix_MultiplyVector(matInvViewProj, rayEnd);
    rayEnd = Vector_Div(rayEnd, rayEnd.w);

    // 4. Ray-Plane Intersection (Plane Y = 0)
    vec3d rayDir = Vector_Normalise(Vector_Sub(rayEnd, rayStart));

    // Safety: Only calculate if we are looking somewhat down/up, not perfectly parallel
    if (abs(rayDir.y) < 0.001f) return { 0,0,0 };

    // t = (PlaneY - StartY) / DirY
    float t = (0.0f - rayStart.y) / rayDir.y;

    // Calculate final intersection point
    vec3d worldPos = Vector_Add(rayStart, Vector_Mul(rayDir, t));
    return worldPos;
}

//------------------------------------------------------------------------
// Called before first update. Do any initial setup here.
//------------------------------------------------------------------------
void Init()
{
    gCoordinator.Init();

    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<MeshComponent>();
    gCoordinator.RegisterComponent<UILabel>();
    gCoordinator.RegisterComponent<UIButton>();
    gCoordinator.RegisterComponent<StatComponent>(); // Verified
    gCoordinator.RegisterComponent<BuilderComponent>();

    // 1. Setup 3D System
    render3D = gCoordinator.RegisterSystem<Render3DSystem>();

    Signature sig3D;
    sig3D.set(gCoordinator.GetComponentType<TransformComponent>());
    sig3D.set(gCoordinator.GetComponentType<MeshComponent>());
    gCoordinator.SetSystemSignature<Render3DSystem>(sig3D);

    // 2. Setup UI System
    renderUI = gCoordinator.RegisterSystem<UIRenderSystem>();
    Signature sigUI;
    sigUI.set(gCoordinator.GetComponentType<UILabel>());
    gCoordinator.SetSystemSignature<UIRenderSystem>(sigUI); // Don't forget to set signature!

    // 3. Setup Button System
    renderButtonUI = gCoordinator.RegisterSystem<UIButtonSystem>();
    Signature sigBtton;
    sigBtton.set(gCoordinator.GetComponentType<UIButton>());
    gCoordinator.SetSystemSignature<UIButtonSystem>(sigBtton);

    // --- ENTITIES ---

    Entity ground = gCoordinator.CreateEntity();
    mesh groundMesh = ShapeBuilder::CreatePlane(50.0f, 0.2f, 0.5f, 0.2f);
    gCoordinator.AddComponent(ground, TransformComponent{ {0, -0.1f, 0} }); // Fixed to -0.1f
    gCoordinator.AddComponent(ground, MeshComponent{ groundMesh });

    // UNITS
    Entity factory = gCoordinator.CreateEntity();
    mesh factoryMesh = ShapeBuilder::CreateFactoryUnit();
    gCoordinator.AddComponent(factory, TransformComponent{ {5, 0, 5} });
    gCoordinator.AddComponent(factory, MeshComponent{ factoryMesh });

    Entity factory2 = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(factory2, TransformComponent{ {5, 0, 10} });
    gCoordinator.AddComponent(factory2, MeshComponent{ factoryMesh });

    Entity factory3 = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(factory3, TransformComponent{ {6, 0, 10} });
    gCoordinator.AddComponent(factory3, MeshComponent{ factoryMesh });

    Entity warrior = gCoordinator.CreateEntity();
    mesh warriorMesh = ShapeBuilder::CreateWarrior();
    gCoordinator.AddComponent(warrior, TransformComponent{ {1, 0, 3} });
    gCoordinator.AddComponent(warrior, MeshComponent{ warriorMesh });

    // --- FIX STARTS HERE ---
    // We must initialize the GLOBAL 'playerStats' entity, not a local 'scoreLabel'
    playerStats = gCoordinator.CreateEntity();

    // 1. Add StatComponent (So we can update score)
    gCoordinator.AddComponent(playerStats, StatComponent{ 0, 100 });

    // 2. Add UILabel (So we can display it)
    gCoordinator.AddComponent(playerStats, UILabel{ 10, 6, "Score: 0", 1, 1, 1 });
    // --- FIX ENDS HERE ---

    // SETUP MOUSE CURSOR
    mouseCursor = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(mouseCursor, TransformComponent{ {0,0,0} });

    // SETUP UI BUTTONS
    btnScore = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(btnScore, UIButton{ 10, 6, 150, 40, "Add Score", 0.2f, 0.6f, 0.2f });

    btnBuild = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(btnBuild, UIButton{ 50, 100, 150, 40, "Build Factory", 0.2f, 0.2f, 0.8f });

    // Camera Setup
    float zoom = 15.0f;
    float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;
    matProj = Engine3D::Matrix_MakeOrthographic(zoom * aspectRatio, zoom, -100.0f, 1000.0f);
    fYaw = 0.785398f;
    fTheta = 0.615472f;
    vFocusPoint = { 0, 0, 0 };
}
//---------------------------------------------------------------------
// Update your simulation here. 
//------------------------------------------------------------------------
void Update(const float deltaTime)
{
float mouseX, mouseY;
    App::GetMousePos(mouseX, mouseY);
    bool isMousePressed = App::IsMousePressed(GLUT_LEFT_BUTTON);
    bool isRightPressed = App::IsMousePressed(GLUT_RIGHT_BUTTON);

	Entity clickedID = renderButtonUI->UpdateInput(mouseX, mouseY, isMousePressed);
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
	// RTS Camera Movement Speed
	float speed = 20.0f * deltaTime / 1000.0f;

	if (App::IsKeyPressed(App::KEY_W)) vFocusPoint.z += speed;
	if (App::IsKeyPressed(App::KEY_S)) vFocusPoint.z -= speed;

	if (App::IsKeyPressed(App::KEY_A)) vFocusPoint.x -= speed;
	if (App::IsKeyPressed(App::KEY_D)) vFocusPoint.x += speed;

	// Calculate fixed Isometric Offset
	float distance = 20.0f;

	// We construct the camera position by rotating a vector {0,0,-dist} 
	// by our fixed Pitch (Theta) and Yaw.
	mat4x4 matPitch = Matrix_MakeRotationX(fTheta);
	mat4x4 matYaw = Matrix_MakeRotationY(fYaw);
	mat4x4 matRot = Matrix_MultiplyMatrix(matPitch, matYaw);

	vec3d vOffset = { 0.0f, 0.0f, -distance };
	vOffset = Matrix_MultiplyVector(matRot, vOffset);

	// Set Camera
	vCamera = Vector_Add(vFocusPoint, vOffset);

	// Removed: fTheta += ... (Stop the spinning!)
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
}
//-------------------------	-----------------------------------------------
// Shutdown
//------------------------------------------------------------------------
void Shutdown()
{
	// Clean up if necessary
}

