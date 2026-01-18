#pragma once
#include "ThreeDVisualiser.h"
#include <vector>

namespace ShapeBuilder
{
    // Helper to merge meshes
    void AddMesh(mesh& target, const mesh& source, vec3d offset, vec3d scale)
    {
        for (auto tri : source.tris)
        {
            // 1. Scale
            tri.p[0].x *= scale.x; tri.p[0].y *= scale.y; tri.p[0].z *= scale.z;
            tri.p[1].x *= scale.x; tri.p[1].y *= scale.y; tri.p[1].z *= scale.z;
            tri.p[2].x *= scale.x; tri.p[2].y *= scale.y; tri.p[2].z *= scale.z;

            // 2. Offset (Move)
            tri.p[0] = Engine3D::Vector_Add(tri.p[0], offset);
            tri.p[1] = Engine3D::Vector_Add(tri.p[1], offset);
            tri.p[2] = Engine3D::Vector_Add(tri.p[2], offset);

            target.tris.push_back(tri);
        }
    }

    // Standard Quad Helper
    void AddQuad(mesh& m, vec3d bl, vec3d br, vec3d tr, vec3d tl, float r, float g, float b)
    {
        m.tris.push_back({ bl, br, tr, r, g, b });
        m.tris.push_back({ bl, tr, tl, r, g, b });
    }

    // CUBE (0 to 1)
    // In your engine: 0 is Top, 1 is Bottom
    mesh CreateCube(float r, float g, float b, bool openTop = false)
    {
        mesh m;
        vec3d p000 = { 0,0,0,1 }; vec3d p100 = { 1,0,0,1 };
        vec3d p010 = { 0,1,0,1 }; vec3d p110 = { 1,1,0,1 };
        vec3d p001 = { 0,0,1,1 }; vec3d p101 = { 1,0,1,1 };
        vec3d p011 = { 0,1,1,1 }; vec3d p111 = { 1,1,1,1 };

        AddQuad(m, p000, p100, p110, p010, r, g, b); // Front
        AddQuad(m, p100, p101, p111, p110, r, g, b); // Right
        AddQuad(m, p101, p001, p011, p111, r, g, b); // Back
        AddQuad(m, p001, p000, p010, p011, r, g, b); // Left

        if (!openTop) AddQuad(m, p010, p110, p111, p011, r, g, b); // Top
        AddQuad(m, p000, p001, p101, p100, r, g, b); // Bottom
        return m;
    }

    // PYRAMID (0 to 1)
    // Base at 1.0 (Bottom), Point at 0.0 (Top)
    // We will shift this UP using negative offsets.
    mesh CreatePyramid(float r, float g, float b)
    {
        mesh m;
        // Point is at 0.0 (The "Top" of the local shape)
        vec3d top = { 0.5f, 0.0f, 0.5f };
        vec3d fl = { 0,1,0 }; vec3d fr = { 1,1,0 };
        vec3d bl = { 0,1,1 }; vec3d br = { 1,1,1 };

        // Winding order: { fl, top, fr }
        m.tris.push_back({ fl, top, fr, r, g, b }); // Front
        m.tris.push_back({ fr, top, br, r, g, b }); // Right
        m.tris.push_back({ br, top, bl, r, g, b }); // Back
        m.tris.push_back({ bl, top, fl, r, g, b }); // Left
        return m;
    }

    // PLANE
    mesh CreatePlane(float size, float r, float g, float b)
    {
        mesh m;
        vec3d p1 = { -size, 0, size };
        vec3d p2 = { size, 0, size };
        vec3d p3 = { size, 0, -size };
        vec3d p4 = { -size, 0, -size };

        m.tris.push_back({ p1, p3, p4, r, g, b });
        m.tris.push_back({ p1, p2, p3, r, g, b });

        return m;
    }

    // --- COMPOSITES ---

    mesh CreateFactoryUnit()
    {
        mesh composite;

        // 1. BASE FIRST (Background)
        // Occupies Y=0 to Y=1
        AddMesh(composite, CreateCube(0.5f, 0.5f, 0.5f, true), { 0,0,0 }, { 1,1,1 });

        // 2. ROOF SECOND (Foreground)
        // Offset Y = -1.0f (Negative means UP).
        // This shifts the pyramid from (0 to 1) -> (-1 to 0).
        // It now sits perfectly on top of the cube.
        AddMesh(composite, CreatePyramid(1.0f, 0.0f, 0.0f), { -0.1f, -1.0f, -0.1f }, { 1.2f, 1.0f, 1.2f });

        return composite;
    }

    mesh CreateWarrior()
    {
        mesh composite;

        // 1. BODY FIRST (Background)
        // Scale Y=0.8. Occupies Y=0 to Y=0.8
        AddMesh(composite, CreateCube(0, 1, 0), { 0,0,0 }, { 0.5f, 0.8f, 0.5f });

        // 2. HEAD SECOND (Foreground)
        // Offset Y = -0.8f (Negative means UP).
        // This shifts the head to sit on top of the body.
        AddMesh(composite, CreateCube(1, 0.8f, 0.6f), { 0.1f, -0.8f, 0.1f }, { 0.3f, 0.3f, 0.3f });

        return composite;
    }
}