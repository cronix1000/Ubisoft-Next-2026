#pragma once
#include "System.h"
#include "Coordinator.h"
#include "../ContestAPI/app.h"
#include <algorithm> 

extern Coordinator gCoordinator;

class Render3DSystem : public System
{
public:
    void Draw(mat4x4& matView, mat4x4& matProj, vec3d cameraPos)
    {
        using namespace Engine3D;

        // 1. SORT ENTITIES (Painter's Algorithm)
        std::vector<Entity> sortedEntities(mEntities.begin(), mEntities.end());

        std::sort(sortedEntities.begin(), sortedEntities.end(),
            [&](Entity a, Entity b) {
                auto& posA = gCoordinator.GetComponent<TransformComponent>(a).Pos;
                auto& posB = gCoordinator.GetComponent<TransformComponent>(b).Pos;
                return Vector_Distance(posA, cameraPos) > Vector_Distance(posB, cameraPos);
            });

        // 2. LIGHTING SETUP
        // CHANGE: Set Light X and Z to negative to match your Camera position (-10, -10, -10).
        // This acts like a "Headlamp" so the faces you see are the ones that get lit.
        vec3d lightSource = { -1.0f, -1.0f, -1.0f };
        vec3d light_dir = Vector_Normalise(lightSource);

        for (auto const& entity : sortedEntities)
        {
            auto& trans = gCoordinator.GetComponent<TransformComponent>(entity);
            auto& meshComp = gCoordinator.GetComponent<MeshComponent>(entity);

            mat4x4 matTrans = Matrix_MakeTranslation(trans.Pos.x, trans.Pos.y, trans.Pos.z);
            mat4x4 matRot = Matrix_MakeRotationY(0.0f);
            mat4x4 matWorld = Matrix_MultiplyMatrix(matRot, matTrans);

            for (auto tri : meshComp.mesh.tris)
            {
                triangle triTransformed, triViewed, triProjected;

                // World
                for (int i = 0; i < 3; i++)
                    triTransformed.p[i] = Matrix_MultiplyVector(matWorld, tri.p[i]);

                // View
                for (int i = 0; i < 3; i++)
                    triViewed.p[i] = Matrix_MultiplyVector(matView, triTransformed.p[i]);

                // Normal & Culling
                vec3d line1 = Vector_Sub(triViewed.p[1], triViewed.p[0]);
                vec3d line2 = Vector_Sub(triViewed.p[2], triViewed.p[0]);
                vec3d normal = Vector_Normalise(Vector_CrossProduct(line1, line2));

                if (normal.z > 0.0f)
                {
                    float dp = Vector_DotProduct(normal, light_dir);

                    // CHANGE: Increase Ambient Light from 0.2f to 0.4f
                    // This ensures even the sides in shadow aren't completely black.
                    if (dp < 0.4f) dp = 0.4f;

                    // Project
                    for (int i = 0; i < 3; i++) {
                        triProjected.p[i] = Matrix_MultiplyVector(matProj, triViewed.p[i]);
                        triProjected.p[i] = Vector_Div(triProjected.p[i], triProjected.p[i].w);
                        triProjected.p[i].x *= -1.0f; triProjected.p[i].y *= -1.0f;
                        triProjected.p[i].x += 1.0f;  triProjected.p[i].y += 1.0f;
                        triProjected.p[i].x *= 0.5f * (float)APP_VIRTUAL_WIDTH;
                        triProjected.p[i].y *= 0.5f * (float)APP_VIRTUAL_HEIGHT;
                    }

                    App::DrawTriangle(
                        triProjected.p[0].x, triProjected.p[0].y, 0, 1,
                        triProjected.p[1].x, triProjected.p[1].y, 0, 1,
                        triProjected.p[2].x, triProjected.p[2].y, 0, 1,
                        tri.r * dp, tri.g * dp, tri.b * dp,
                        tri.r * dp, tri.g * dp, tri.b * dp,
                        tri.r * dp, tri.g * dp, tri.b * dp,
                        false
                    );
                }
            }
        }
    }
};