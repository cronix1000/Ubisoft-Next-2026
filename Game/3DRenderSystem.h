#pragma once
#include "System.h"
#include "Coordinator.h"
#include "../ContestAPI/app.h"
#include <algorithm> 
#include <vector>

extern Coordinator gCoordinator;

class Render3DSystem : public System
{
    struct RenderTri
    {
        triangle t;
        float depth;
        vec3d normal;
        float lightFactor;
    };

    // OPTIMIZATION 1: Persistent Memory (Prevents lag spikes from memory allocation)
    std::vector<RenderTri> m_trianglesCache;

public:
    void Init() {
        m_trianglesCache.reserve(20000);
    }

    void Draw(mat4x4& matView, mat4x4& matProj, vec3d cameraPos)
    {
        using namespace Engine3D;

        m_trianglesCache.clear(); // Reset count, keep memory

        vec3d lightDir = { 0.5f, -1.0f, 1.0f };
        lightDir = Vector_Normalise(lightDir);

        for (auto const& entity : mEntities)
        {
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& meshComp = gCoordinator.GetComponent<MeshComponent>(entity);

            // --- SMART LOD SYSTEM ---
            float distToCamera = Vector_Distance(trans.Pos, cameraPos);
            size_t totalTris = meshComp.mesh.tris.size();

            // HEURISTIC: Identify different mesh types for proper culling
            bool isGroundPlane = (trans.Pos.y < 0.0f);
            bool isSimpleMesh = (totalTris <= 12);

            // Rule 1: Culling (Hide things far away)
            // Never cull ground plane or simple decorative meshes
            if (!meshComp.isImportant && distToCamera > 180.0f) continue;

            // Rule 2: Level of Detail (Simplify units far away)
            size_t drawLimit = totalTris;
            if (!isSimpleMesh && distToCamera > 50.0f) {
                drawLimit = (std::min)(totalTris, (size_t)12);
            }
            // ------------------------

            mat4x4 matTrans = Matrix_MakeTranslation(trans.Pos.x, trans.Pos.y, trans.Pos.z);
            mat4x4 matRot = Matrix_MakeRotationY(0.0f);
            mat4x4 matWorld = Matrix_MultiplyMatrix(matRot, matTrans);

            for (size_t i = 0; i < drawLimit; i++)
            {
                auto& tri = meshComp.mesh.tris[i];
                RenderTri rt;

                // Transform to World Space
                rt.t.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
                rt.t.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
                rt.t.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

                vec3d line1 = Vector_Sub(rt.t.p[1], rt.t.p[0]);
                vec3d line2 = Vector_Sub(rt.t.p[2], rt.t.p[0]);
                rt.normal = Vector_Normalise(Vector_CrossProduct(line1, line2));

                vec3d center = Vector_Div(Vector_Add(rt.t.p[0], Vector_Add(rt.t.p[1], rt.t.p[2])), 3.0f);
                vec3d camRay = Vector_Sub(center, cameraPos);

                // Backface Culling
                if (Vector_DotProduct(rt.normal, camRay) < 0.0f)
                {
                    float dp = fabsf(Vector_DotProduct(rt.normal, lightDir));
                    float ambient = 0.3f;
                    rt.lightFactor = (std::max)(ambient, dp);
                    rt.t.r = tri.r; rt.t.g = tri.g; rt.t.b = tri.b;
                    rt.depth = Vector_Distance(center, cameraPos);

                    m_trianglesCache.push_back(rt);
                }
            }
        }

        // Sort Painter's Algorithm
        std::sort(m_trianglesCache.begin(), m_trianglesCache.end(),
            [](const RenderTri& a, const RenderTri& b) {
                return a.depth > b.depth;
            });

        // Rasterize
        for (auto& rt : m_trianglesCache)
        {
            triangle triViewed, triProjected;

            triViewed.p[0] = Matrix_MultiplyVector(matView, rt.t.p[0]);
            triViewed.p[1] = Matrix_MultiplyVector(matView, rt.t.p[1]);
            triViewed.p[2] = Matrix_MultiplyVector(matView, rt.t.p[2]);

            // Near Plane Clip
            if (triViewed.p[0].z < 0.1f || triViewed.p[1].z < 0.1f || triViewed.p[2].z < 0.1f) continue;

            for (int i = 0; i < 3; i++)
            {
                triProjected.p[i] = Matrix_MultiplyVector(matProj, triViewed.p[i]);
                triProjected.p[i].y *= 1.0f;
                triProjected.p[i] = Vector_Div(triProjected.p[i], triProjected.p[i].w);
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