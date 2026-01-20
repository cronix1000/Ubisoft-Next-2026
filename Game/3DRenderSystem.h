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

    // OPTIMIZATION 1: Persistent Memory
    // Moving this here prevents the game from asking the RAM for new memory 60 times a second.
    std::vector<RenderTri> m_trianglesCache;

public:
    void Init() {
        m_trianglesCache.reserve(20000); // Pre-allocate memory for ~1000 units
    }

    void Draw(mat4x4& matView, mat4x4& matProj, vec3d cameraPos)
    {
        using namespace Engine3D;

        // Reset the count, but keep the memory allocated
        m_trianglesCache.clear();

        // 1. LIGHTING DIRECTION
        vec3d lightDir = { 0.5f, -1.0f, 1.0f };
        lightDir = Vector_Normalise(lightDir);

        // 2. GATHER AND TRANSFORM
        for (auto const& entity : mEntities)
        {
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& meshComp = gCoordinator.GetComponent<MeshComponent>(entity);

            mat4x4 matTrans = Matrix_MakeTranslation(trans.Pos.x, trans.Pos.y, trans.Pos.z);
            mat4x4 matRot = Matrix_MakeRotationY(0.0f);
            mat4x4 matWorld = Matrix_MultiplyMatrix(matRot, matTrans);

            // Process all triangles in the mesh
            for (size_t i = 0; i < meshComp.mesh.tris.size(); i++)
            {
                auto& tri = meshComp.mesh.tris[i];
                RenderTri rt;

                // Transform to World Space
                rt.t.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
                rt.t.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
                rt.t.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

                // Calculate Center
                vec3d center = Vector_Div(Vector_Add(rt.t.p[0], Vector_Add(rt.t.p[1], rt.t.p[2])), 3.0f);

                // Cull Logic: Is the triangle facing the camera?
                vec3d camRay = Vector_Sub(center, cameraPos);

                // Calculate Normal
                vec3d line1 = Vector_Sub(rt.t.p[1], rt.t.p[0]);
                vec3d line2 = Vector_Sub(rt.t.p[2], rt.t.p[0]);
                rt.normal = Vector_Normalise(Vector_CrossProduct(line1, line2));

                if (Vector_DotProduct(rt.normal, camRay) < 0.0f)
                {
                    // Lighting
                    float dp = fabsf(Vector_DotProduct(rt.normal, lightDir));
                    float ambient = 0.3f;
                    rt.lightFactor = (std::max)(ambient, dp);

                    rt.t.r = tri.r; rt.t.g = tri.g; rt.t.b = tri.b;
                    rt.depth = Vector_Distance(center, cameraPos);

                    m_trianglesCache.push_back(rt);
                }
            }
        }

        // 3. SORT
        std::sort(m_trianglesCache.begin(), m_trianglesCache.end(),
            [](const RenderTri& a, const RenderTri& b) {
                return a.depth > b.depth;
            });

        // 4. PROJECT AND DRAW
        for (auto& rt : m_trianglesCache)
        {
            triangle triViewed, triProjected;

            // World -> View
            triViewed.p[0] = Matrix_MultiplyVector(matView, rt.t.p[0]);
            triViewed.p[1] = Matrix_MultiplyVector(matView, rt.t.p[1]);
            triViewed.p[2] = Matrix_MultiplyVector(matView, rt.t.p[2]);

            // Clip Near Plane
            if (triViewed.p[0].z < 0.1f || triViewed.p[1].z < 0.1f || triViewed.p[2].z < 0.1f)
                continue;

            // View -> Projected
            for (int i = 0; i < 3; i++)
            {
                triProjected.p[i] = Matrix_MultiplyVector(matProj, triViewed.p[i]);
                triProjected.p[i].y *= 1.0f;
                triProjected.p[i] = Vector_Div(triProjected.p[i], triProjected.p[i].w);

                // Screen Scale
                triProjected.p[i].x += 1.0f; triProjected.p[i].y += 1.0f;
                triProjected.p[i].x *= 0.5f * (float)APP_VIRTUAL_WIDTH;
                triProjected.p[i].y *= 0.5f * (float)APP_VIRTUAL_HEIGHT;
            }

            float r = rt.t.r * rt.lightFactor;
            float g = rt.t.g * rt.lightFactor;
            float b = rt.t.b * rt.lightFactor;

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