
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
	float zoom = 20.0f;
	float aspectRatio = (float)APP_VIRTUAL_WIDTH / (float)APP_VIRTUAL_HEIGHT;

	// Create the matrix
	matProj = Engine3D::Matrix_MakeOrthographic(zoom * aspectRatio, zoom, -100.0f, 1000.0f);

	fYaw = 0.785f;   // 45 degrees in radians
	fTheta = 0.615f; // ~35 degrees down (Standard Iso Angle)
}

//------------------------------------------------------------------------
// Update your simulation here. 
//------------------------------------------------------------------------
void Update(const float deltaTime)
{
	float speed = 10.0f * deltaTime / 1000.0f;
	// ... (keep your input controls here) ...

	// FIX: Set Y to POSITIVE 10.0f.
	// In a Y-Up world, this places the camera in the sky, looking down.
	vec3d vOffset = { -10.0f, 10.0f, -10.0f };
	vec3d vUp = { 0,1,0 };
	vec3d vTarget = { 0,0,1 };

	// ... (rest of the function remains the same) ...
	mat4x4 matCameraRot = Matrix_MakeRotationY(fYaw);
	vTarget = Vector_Add(vCamera, vLookDir);
	vCamera = Vector_Add(vFocusPoint, vOffset);
	fTheta += 1.0f * deltaTime / 1000.0f;
}
//------------------------------------------------------------------------
// Display calls here 
//------------------------------------------------------------------------
void Render()
{
	// 1. Update Camera Matrix using the GLOBAL vCamera variable
	// This allows the Update() function to actually move the camera.
	vec3d vTarget = vFocusPoint;
	vec3d vUp = { 0.0f, 1.0f, 0.0f };

	mat4x4 matCamera = Matrix_PointAt(vCamera, vTarget, vUp);
	mat4x4 matView = Matrix_QuickInverse(matCamera);

	// 2. Draw
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