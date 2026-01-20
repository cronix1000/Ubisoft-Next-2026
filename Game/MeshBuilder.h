#pragma once
#include "ThreeDVisualiser.h"
#include <vector>
#include <cmath>

#ifndef PI
#define PI 3.14159265359f
#endif

namespace ShapeBuilder
{
    vec3d RotatePointX(vec3d p, float angle) {
        vec3d n = p;
        n.y = p.y * cos(angle) - p.z * sin(angle);
        n.z = p.y * sin(angle) + p.z * cos(angle);
        return n;
    }

    vec3d RotatePointY(vec3d p, float angle) {
        vec3d n = p;
        n.x = p.x * cos(angle) + p.z * sin(angle);
        n.z = -p.x * sin(angle) + p.z * cos(angle);
        return n;
    }

    vec3d RotatePointZ(vec3d p, float angle) {
        vec3d n = p;
        n.x = p.x * cos(angle) - p.y * sin(angle);
        n.y = p.x * sin(angle) + p.y * cos(angle);
        return n;
    }

    void RotateMesh(mesh& m, float xAngle, float yAngle, float zAngle) {
        for (auto& tri : m.tris) {
            for (int i = 0; i < 3; i++) {
                if (xAngle != 0) tri.p[i] = RotatePointX(tri.p[i], xAngle);
                if (yAngle != 0) tri.p[i] = RotatePointY(tri.p[i], yAngle);
                if (zAngle != 0) tri.p[i] = RotatePointZ(tri.p[i], zAngle);
            }
        }
    }

    void AddMesh(mesh& target, const mesh& source, vec3d offset, vec3d scale)
    {
        for (auto tri : source.tris)
        {
            tri.p[0].x *= scale.x; tri.p[0].y *= scale.y; tri.p[0].z *= scale.z;
            tri.p[1].x *= scale.x; tri.p[1].y *= scale.y; tri.p[1].z *= scale.z;
            tri.p[2].x *= scale.x; tri.p[2].y *= scale.y; tri.p[2].z *= scale.z;

            tri.p[0] = Engine3D::Vector_Add(tri.p[0], offset);
            tri.p[1] = Engine3D::Vector_Add(tri.p[1], offset);
            tri.p[2] = Engine3D::Vector_Add(tri.p[2], offset);

            target.tris.push_back(tri);
        }
    }

    void TintMesh(mesh& m, float r, float g, float b) {
        for (auto& tri : m.tris) {
            tri.r = r; tri.g = g; tri.b = b;
        }
    }

    void AddQuad(mesh& m, vec3d bl, vec3d br, vec3d tr, vec3d tl, float r, float g, float b)
    {
        m.tris.push_back({ bl, br, tr, r, g, b });
        m.tris.push_back({ bl, tr, tl, r, g, b });
    }

    mesh CreateCube(float r, float g, float b, bool openTop = false)
    {
        mesh m;
        vec3d p000 = { 0,0,0,1 }; vec3d p100 = { 1,0,0,1 };
        vec3d p010 = { 0,1,0,1 }; vec3d p110 = { 1,1,0,1 };
        vec3d p001 = { 0,0,1,1 }; vec3d p101 = { 1,0,1,1 };
        vec3d p011 = { 0,1,1,1 }; vec3d p111 = { 1,1,1,1 };

        AddQuad(m, p010, p110, p100, p000, r, g, b); // Front
        AddQuad(m, p001, p101, p111, p011, r, g, b); // Back
        AddQuad(m, p110, p111, p101, p100, r * 0.9f, g * 0.9f, b * 0.9f); // Right (slightly darker)
        AddQuad(m, p000, p001, p011, p010, r * 0.9f, g * 0.9f, b * 0.9f); // Left
        if (!openTop) AddQuad(m, p010, p011, p111, p110, r * 1.1f, g * 1.1f, b * 1.1f); // Top (lighter)
        AddQuad(m, p000, p100, p101, p001, r * 0.8f, g * 0.8f, b * 0.8f); // Bottom (darkest)

        return m;
    }

    mesh CreateHexagon(float r, float g, float b, float height = 1.0f)
    {
        mesh m;
        float radius = 0.5f;
        float centerX = 0.5f; float centerZ = 0.5f;
        vec3d bottomVerts[6]; vec3d topVerts[6];

        for (int i = 0; i < 6; i++) {
            float angle = (PI / 3.0f) * i;
            float x = centerX + radius * cos(angle);
            float z = centerZ + radius * sin(angle);
            bottomVerts[i] = { x, 0.0f, z, 1 };
            topVerts[i] = { x, height, z, 1 };
        }

        vec3d bottomCenter = { centerX, 0.0f, centerZ, 1 };
        vec3d topCenter = { centerX, height, centerZ, 1 };

        for (int i = 0; i < 6; i++) {
            int next = (i + 1) % 6;
            m.tris.push_back({ bottomCenter, bottomVerts[next], bottomVerts[i], r * 0.8f, g * 0.8f, b * 0.8f });
            m.tris.push_back({ topCenter, topVerts[i], topVerts[next], r * 1.1f, g * 1.1f, b * 1.1f });
            AddQuad(m, bottomVerts[i], bottomVerts[next], topVerts[next], topVerts[i], r, g, b);
        }
        return m;
    }

    mesh CreatePyramid(float r, float g, float b)
    {
        mesh m;
        vec3d top = { 0.5f, 1.0f, 0.5f };
        vec3d fl = { 0,0,0 }; vec3d fr = { 1,0,0 };
        vec3d bl = { 0,0,1 }; vec3d br = { 1,0,1 };
        m.tris.push_back({ fl, top, fr, r, g, b });
        m.tris.push_back({ fr, top, br, r * 0.9f, g * 0.9f, b * 0.9f });
        m.tris.push_back({ br, top, bl, r, g, b });
        m.tris.push_back({ bl, top, fl, r * 0.9f, g * 0.9f, b * 0.9f });
        AddQuad(m, fl, fr, br, bl, r * 0.8f, g * 0.8f, b * 0.8f);
        return m;
    }

    mesh CreateCubeScales(vec3d scale, float r, float g, float b, bool openTop = false) {

        mesh composite;
        AddMesh(composite, CreateCube(0.2f, 0.2f, 0.2f), { 0,0,0 }, scale);
        return composite;
    }

    mesh CreateHexagonScales(vec3d scale, float r, float g, float b, float height = 1.0f) {

        mesh composite;
        AddMesh(composite, CreateHexagon(r, g, b, height), { 0,0,0 }, scale);
        return composite;

    }

    enum UnitType { MELEE, RANGED };

    // Triangles ordered by importance: Core body -> Head -> Limbs -> Details
    mesh CreateDetailedUnit(UnitType type, float r, float g, float b) {
        mesh composite;

        float skinR = 0.9f, skinG = 0.7f, skinB = 0.6f;
        float darkR = 0.2f, darkG = 0.2f, darkB = 0.2f;
        
        AddMesh(composite, CreateCube(r, g, b), { 0.1f, 0.4f, 0.15f }, { 0.8f, 0.7f, 0.4f });

        AddMesh(composite, CreateCube(skinR, skinG, skinB), { 0.3f, 1.1f, 0.25f }, { 0.4f, 0.4f, 0.4f });
        
        AddMesh(composite, CreateCube(darkR, darkG, darkB), { 0.1f, 0.0f, 0.2f }, { 0.25f, 0.4f, 0.25f }); // Left
        AddMesh(composite, CreateCube(darkR, darkG, darkB), { 0.65f, 0.0f, 0.2f }, { 0.25f, 0.4f, 0.25f }); // Right

        // 4. Arms - 24 triangles
        AddMesh(composite, CreateCube(r, g, b), { -0.15f, 0.7f, 0.2f }, { 0.25f, 0.6f, 0.25f }); // Left Arm
        AddMesh(composite, CreateCube(r, g, b), { 0.9f, 0.7f, 0.2f }, { 0.25f, 0.6f, 0.25f });  // Right Arm

        if (type == MELEE) {
            // Helmet (Pyramid on head)
            AddMesh(composite, CreatePyramid(0.7f, 0.7f, 0.7f), { 0.3f, 1.5f, 0.25f }, { 0.4f, 0.3f, 0.4f });

            // Sword (Long thin blade + crossguard + hilt)
            mesh sword;
            AddMesh(sword, CreateCube(0.4f, 0.2f, 0.1f), { 0, 0, 0 }, { 0.05f, 0.3f, 0.05f }); // Hilt
            AddMesh(sword, CreateCube(0.8f, 0.8f, 0.1f), { -0.1f, 0.3f, -0.05f }, { 0.25f, 0.05f, 0.15f }); // Guard
            AddMesh(sword, CreateCube(0.9f, 0.9f, 0.95f), { -0.025f, 0.35f, -0.025f }, { 0.1f, 0.8f, 0.1f }); // Blade
            RotateMesh(sword, 0.5f, 0, 0.3f); // Tilt sword forward and out
            AddMesh(composite, sword, { 1.1f, 0.6f, 0.4f }, { 1,1,1 });

            // Shield (Left hand)
            mesh shield;
            AddMesh(shield, CreateCube(r * 0.8f, g * 0.8f, b * 0.8f), { 0,0,0 }, { 0.1f, 0.6f, 0.6f });
            AddMesh(shield, CreateCube(0.9f, 0.9f, 0.9f), { -0.01f, 0.1f, 0.1f }, { 0.1f, 0.4f, 0.4f }); // Trim
            AddMesh(composite, shield, { -0.15f, 0.5f, 0.2f }, { 1,1,1 });
        }
        else if (type == RANGED) {
            // Hood (Green/Brown pyramid)
            AddMesh(composite, CreatePyramid(0.2f, 0.3f, 0.1f), { 0.3f, 1.5f, 0.25f }, { 0.4f, 0.2f, 0.4f });

            // Bow (Curved shape approximated by 3 blocks)
            mesh bow;
            // Middle
            AddMesh(bow, CreateCube(0.4f, 0.2f, 0.1f), { 0, 0, 0 }, { 0.05f, 0.8f, 0.05f });
            // Top limb (rotated)
            mesh topLimb = CreateCube(0.4f, 0.2f, 0.1f);
            RotateMesh(topLimb, 0, 0, 0.3f); // Angle in Z
            AddMesh(bow, topLimb, { -0.1f, 0.8f, 0 }, { 0.05f, 0.5f, 0.05f });
            // Bottom limb (rotated)
            mesh botLimb = CreateCube(0.4f, 0.2f, 0.1f);
            RotateMesh(botLimb, 0, 0, -0.3f);
            AddMesh(bow, botLimb, { -0.1f, -0.5f, 0 }, { 0.05f, 0.5f, 0.05f });

            // Attach bow to left hand
            AddMesh(composite, bow, { -0.3f, 0.8f, 0.5f }, { 1,1,1 });
        }

        return composite;
    }

    mesh CreateWarrior() {
        return CreateDetailedUnit(MELEE, 0.2f, 0.4f, 0.8f); // Blue armor
    }

    mesh CreateEnemyWarrior() {
        return CreateDetailedUnit(MELEE, 0.8f, 0.2f, 0.2f); // Red armor
    }

    mesh CreateRangedWarrior() {
        return CreateDetailedUnit(RANGED, 0.4f, 0.6f, 0.2f); // Green/Hunter tunic
    }

    mesh CreateEnemyRangedWarrior() {
        return CreateDetailedUnit(RANGED, 0.8f, 0.2f, 0.2f);
    }

    mesh CreateCatapult()
    {
        mesh composite;
        float woodR = 0.5f, woodG = 0.35f, woodB = 0.1f;
        float wheelR = 0.3f, wheelG = 0.3f, wheelB = 0.3f;

        AddMesh(composite, CreateCube(woodR, woodG, woodB), { 0.1f, 0.2f, 0.0f }, { 0.1f, 0.1f, 1.0f });
        AddMesh(composite, CreateCube(woodR, woodG, woodB), { 0.8f, 0.2f, 0.0f }, { 0.1f, 0.1f, 1.0f });

        AddMesh(composite, CreateCube(woodR, woodG, woodB), { 0.1f, 0.2f, 0.1f }, { 0.8f, 0.1f, 0.1f });
        AddMesh(composite, CreateCube(woodR, woodG, woodB), { 0.1f, 0.2f, 0.8f }, { 0.8f, 0.1f, 0.1f });

        mesh wheel = CreateHexagon(wheelR, wheelG, wheelB, 0.2f);
        RotateMesh(wheel, PI / 2.0f, 0, 0); // Rotate to stand up (around X)
        RotateMesh(wheel, 0, PI / 2.0f, 0); // Rotate to face forward (around Y)

        // Wheel placement
        vec3d wheelScale = { 0.5f, 0.5f, 0.5f };
        AddMesh(composite, wheel, { -0.1f, 0.0f, 0.1f }, wheelScale); // Front Left
        AddMesh(composite, wheel, { 0.9f, 0.0f, 0.1f }, wheelScale);  // Front Right
        AddMesh(composite, wheel, { -0.1f, 0.0f, 0.8f }, wheelScale); // Back Left
        AddMesh(composite, wheel, { 0.9f, 0.0f, 0.8f }, wheelScale);  // Back Right

        // 3. The Arm Structure (Triangular support)
        AddMesh(composite, CreatePyramid(woodR, woodG, woodB), { 0.1f, 0.3f, 0.4f }, { 0.1f, 0.5f, 0.2f }); // Left Support
        AddMesh(composite, CreatePyramid(woodR, woodG, woodB), { 0.8f, 0.3f, 0.4f }, { 0.1f, 0.5f, 0.2f }); // Right Support

        // 4. The Firing Arm (Angled)
        mesh arm;
        // The beam
        AddMesh(arm, CreateCube(woodR * 1.2f, woodG * 1.2f, woodB * 1.2f), { 0,0,0 }, { 0.1f, 0.05f, 0.9f });
        // The bucket at the end
        AddMesh(arm, CreateCube(woodR, woodG, woodB, true), { -0.05f, 0.0f, 0.85f }, { 0.2f, 0.15f, 0.2f });
        // The stone inside
        AddMesh(arm, CreateCube(0.2f, 0.2f, 0.2f), { -0.02f, 0.05f, 0.9f }, { 0.15f, 0.15f, 0.15f });

        // Angle the whole arm up
        RotateMesh(arm, -PI / 4.0f, 0, 0); // Tilt back 45 degrees

        // Attach arm to pivot point
        AddMesh(composite, arm, { 0.45f, 0.6f, 0.5f }, { 1,1,1 });

        return composite;
    }

    mesh CreateTree() {
        mesh composite;
        AddMesh(composite, CreateCube(0.55f, 0.27f, 0.07f), { 0.35f, 0.0f, 0.35f }, { 0.3f, 0.4f, 0.3f });

        AddMesh(composite, CreatePyramid(0.1f, 0.6f, 0.1f), { 0.1f, 0.3f, 0.1f }, { 0.8f, 0.6f, 0.8f });
        AddMesh(composite, CreatePyramid(0.15f, 0.7f, 0.15f), { 0.15f, 0.6f, 0.15f }, { 0.7f, 0.6f, 0.7f });
        AddMesh(composite, CreatePyramid(0.2f, 0.8f, 0.2f), { 0.2f, 0.9f, 0.2f }, { 0.6f, 0.6f, 0.6f });
        return composite;
    }

    mesh CreateFactoryUnit() {
        mesh composite;
        AddMesh(composite, CreateCube(0.5f, 0.5f, 0.5f, true), { -0.5f, 0.0f, -0.5f }, { 1,1,1 });
        AddMesh(composite, CreatePyramid(0.6f, 0.6f, 0.6f), { -0.5f, 1.0f, -0.5f }, { 1.0f, 0.5f, 1.0f });
        AddMesh(composite, CreateCube(0.3f, 0.3f, 0.3f), { 0.1f, 0.5f, -0.4f }, { 0.2f, 1.0f, 0.2f });
        return composite;
    }

    mesh CreatePlane(float size, float r, float g, float b)
    {
        mesh m;
        int tiles = (int)size;
        if (tiles < 10) tiles = 10;
        if (tiles > 100) tiles = 100;

        float step = (size * 2.0f) / tiles;
        float start = -size;

        for (int x = 0; x < tiles; x++)
        {
            for (int z = 0; z < tiles; z++)
            {
                float x0 = start + (x * step);
                float x1 = start + ((x + 1) * step);
                float z0 = start + (z * step);
                float z1 = start + ((z + 1) * step);

                // Checkerboard pattern tint
                float tint = ((x + z) % 2 == 0) ? 1.0f : 0.9f;

                vec3d p1 = { x0, 0, z1 }; // Bottom Left
                vec3d p2 = { x1, 0, z1 }; // Bottom Right
                vec3d p3 = { x1, 0, z0 }; // Top Right
                vec3d p4 = { x0, 0, z0 }; // Top Left

                AddQuad(m, p1, p2, p3, p4, r * tint, g * tint, b * tint);
            }
        }
        return m;
    }
}