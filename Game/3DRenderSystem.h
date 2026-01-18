#pragma once
#include "System.h"
#include "Coordinator.h"
#include "../ContestAPI/app.h"
#include <algorithm> 
#include <vector>

extern Coordinator gCoordinator;

class Render3DSystem : public System
{
    // Helper struct for sorting
    struct RenderTri
    {
        triangle t; // World Space Triangle
        float depth;          // Average Z for sorting
        vec3d normal;
        float lightFactor;    // Pre-calculated lighting
    };

public:
    void Draw(mat4x4& matView, mat4x4& matProj, vec3d cameraPos)
    {
        using namespace Engine3D;

        std::vector<RenderTri> trianglesToRaster;

        // 1. LIGHTING DIRECTION (Direction FROM light TO world)
        // Pointing Down and Forward
        vec3d lightDir = { 0.5f, -1.0f, 1.0f };
        lightDir = Vector_Normalise(lightDir);

        // 2. GATHER AND TRANSFORM TO WORLD SPACE
        for (auto const& entity : mEntities)
        {
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& meshComp = gCoordinator.GetComponent<MeshComponent>(entity);

            mat4x4 matTrans = Matrix_MakeTranslation(trans.Pos.x, trans.Pos.y, trans.Pos.z);
            mat4x4 matRot = Matrix_MakeRotationY(0.0f); // Add rotation if needed
            mat4x4 matWorld = Matrix_MultiplyMatrix(matRot, matTrans);

            for (auto tri : meshComp.mesh.tris)
            {
                RenderTri rt;

                // Transform to World Space
                rt.t.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
                rt.t.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
                rt.t.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

                // Calculate Normal (World Space)
                vec3d line1 = Vector_Sub(rt.t.p[1], rt.t.p[0]);
                vec3d line2 = Vector_Sub(rt.t.p[2], rt.t.p[0]);
                rt.normal = Vector_Normalise(Vector_CrossProduct(line1, line2));

                // Calculate Center for Sorting
                vec3d center = Vector_Div(Vector_Add(rt.t.p[0], Vector_Add(rt.t.p[1], rt.t.p[2])), 3.0f);

                // Cull Logic: Is the triangle facing the camera?
                // Vector from Camera to Triangle
                vec3d camRay = Vector_Sub(center, cameraPos);

                // If Dot Product is < 0, the face is looking AT the camera (Visible)
                if (Vector_DotProduct(rt.normal, camRay) < 0.0f)
                {
                    // Calculate Lighting (Double Sided)
                    float dp = fabsf(Vector_DotProduct(rt.normal, lightDir));
                    float ambient = 0.3f;
                    rt.lightFactor = (std::max)(ambient, dp);

                    // Copy Colors
                    rt.t.r = tri.r; rt.t.g = tri.g; rt.t.b = tri.b;

                    // Calculate Depth (Distance from Camera)
                    rt.depth = Vector_Distance(center, cameraPos);

                    trianglesToRaster.push_back(rt);
                }
            }
        }

        // 3. SORT TRIANGLES 
        // (Still useful for transparency, but Z-buffer handles the hard work now)
        std::sort(trianglesToRaster.begin(), trianglesToRaster.end(),
            [](const RenderTri& a, const RenderTri& b) {
                return a.depth > b.depth;
            });

        // 4. PROJECT AND DRAW
        for (auto& rt : trianglesToRaster)
        {
            triangle triViewed, triProjected;

            // World -> View
            triViewed.p[0] = Matrix_MultiplyVector(matView, rt.t.p[0]);
            triViewed.p[1] = Matrix_MultiplyVector(matView, rt.t.p[1]);
            triViewed.p[2] = Matrix_MultiplyVector(matView, rt.t.p[2]);

            // CLIP: Simple Near Plane Check to prevent glitches
            if (triViewed.p[0].z < 0.1f || triViewed.p[1].z < 0.1f || triViewed.p[2].z < 0.1f)
                continue;

            // View -> Projected
            for (int i = 0; i < 3; i++)
            {
                triProjected.p[i] = Matrix_MultiplyVector(matProj, triViewed.p[i]);


                triProjected.p[i].y *= 1.0f;
                // Perspective Divide
                triProjected.p[i] = Vector_Div(triProjected.p[i], triProjected.p[i].w);

                // Screen Scale
                triProjected.p[i].x += 1.0f; triProjected.p[i].y += 1.0f;
                triProjected.p[i].x *= 0.5f * (float)APP_VIRTUAL_WIDTH;
                triProjected.p[i].y *= 0.5f * (float)APP_VIRTUAL_HEIGHT;
            }

            // Apply Lighting to Color
            float r = rt.t.r * rt.lightFactor;
            float g = rt.t.g * rt.lightFactor;
            float b = rt.t.b * rt.lightFactor;

            // FIX: Pass the Z depth (triProjected.p[].z) so the ground doesn't overwrite the units
            App::DrawTriangle(
                triProjected.p[0].x, triProjected.p[0].y, triProjected.p[0].z, 1.0f,
                triProjected.p[1].x, triProjected.p[1].y, triProjected.p[1].z, 1.0f,
                triProjected.p[2].x, triProjected.p[2].y, triProjected.p[2].z, 1.0f,
                r, g, b, r, g, b, r, g, b,
                false
            );
        }
    }
};