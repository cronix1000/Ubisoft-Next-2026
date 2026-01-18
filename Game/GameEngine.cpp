
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

using namespace Engine3D;

Coordinator gCoordinator;

std::shared_ptr<Render3DSystem> render3D;
std::shared_ptr<UIRenderSystem> renderUI;
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

//------------------------------------------------------------------------
// Called before first update. Do any initial setup here.
//------------------------------------------------------------------------
void Init()
{
	gCoordinator.Init();

	gCoordinator.RegisterComponent<TransformComponent>();
	gCoordinator.RegisterComponent<MeshComponent>();
	gCoordinator.RegisterComponent<UILabel>();

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
	gCoordinator.SetSystemSignature<UIRenderSystem>(sigUI);
	Entity ground = gCoordinator.CreateEntity();

	// Y = -0.1f puts it just below the units
	mesh groundMesh = ShapeBuilder::CreatePlane(50.0f, 0.2f, 0.5f, 0.2f);
	gCoordinator.AddComponent(ground, TransformComponent{ {0, 0.1f, 0} });
	gCoordinator.AddComponent(ground, MeshComponent{ groundMesh });

	// 2. CREATE UNITS (Draws on top of ground)
	Entity factory = gCoordinator.CreateEntity();
	mesh factoryMesh = ShapeBuilder::CreateFactoryUnit();
	gCoordinator.AddComponent(factory, TransformComponent{ {5, 0, 5} });
	gCoordinator.AddComponent(factory, MeshComponent{ factoryMesh });

	mesh factoryMesh2 = ShapeBuilder::CreateFactoryUnit();
	Entity factory2 = gCoordinator.CreateEntity();
	gCoordinator.AddComponent(factory2, TransformComponent{ {5, 0, 10} });
	gCoordinator.AddComponent(factory2, MeshComponent{ factoryMesh2 });	
	
	Entity factory3 = gCoordinator.CreateEntity();
	gCoordinator.AddComponent(factory3, TransformComponent{ {6, 0, 10} });
	gCoordinator.AddComponent(factory3, MeshComponent{ factoryMesh2 });

	Entity warrior = gCoordinator.CreateEntity();
	mesh warriorMesh = ShapeBuilder::CreateWarrior();
	gCoordinator.AddComponent(warrior, TransformComponent{ {1, 0, 3} });
	gCoordinator.AddComponent(warrior, MeshComponent{ warriorMesh });

	// 4. Create a UI Entity (Score)
	Entity scoreLabel = gCoordinator.CreateEntity();
	gCoordinator.AddComponent(scoreLabel, UILabel{ 10, 6, "Score: 0", 1, 1, 1  });

	// 20.0f is the "Zoom Level". Smaller number = Zoom In. Larger = Zoom Out.
	float zoom = 15.0f;
	float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;

	vFocusPoint = { 0, 0, 0 };
	vCamera = { -10.0f, 10.0f, -10.0f };
	// RTS usually uses Orthographic for consistent unit sizes
	matProj = Engine3D::Matrix_MakeOrthographic(zoom * aspectRatio, zoom, -100.0f, 1000.0f);

	// Standard Isometric Angles
	fYaw = 0.785398f;   // 45 degrees
	fTheta = 0.615472f; // ~35.26 degrees (asin(tan(30)))

	// Set initial focus point
	vFocusPoint = { 0, 0, 0 };
}

//------------------------------------------------------------------------
// Update your simulation here. 
//------------------------------------------------------------------------
void Update(const float deltaTime)
{
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
//------------------------------------------------------------------------
// Shutdown
//------------------------------------------------------------------------
void Shutdown()
{
	// Clean up if necessary
}