#pragma once
#include "ThreeDVisualiser.h"
#include <vector>

namespace ShapeBuilder
{
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

    void AddQuad(mesh& m, vec3d bl, vec3d br, vec3d tr, vec3d tl, float r, float g, float b)
    {
        m.tris.push_back({ bl, br, tr, r, g, b });
        m.tris.push_back({ bl, tr, tl, r, g, b });
    }

    // CUBE
    mesh CreateCube(float r, float g, float b, bool openTop = false)
    {
        mesh m;
        vec3d p000 = { 0,0,0,1 }; vec3d p100 = { 1,0,0,1 };
        vec3d p010 = { 0,1,0,1 }; vec3d p110 = { 1,1,0,1 };
        vec3d p001 = { 0,0,1,1 }; vec3d p101 = { 1,0,1,1 };

        // FIX: Fixed typo (was {1,1,1,1}, now {0,1,1,1})
        vec3d p011 = { 0,1,1,1 };
        vec3d p111 = { 1,1,1,1 };

        AddQuad(m, p010, p110, p100, p000, r, g, b); // Front
        AddQuad(m, p001, p101, p111, p011, r, g, b); // Back
        AddQuad(m, p110, p111, p101, p100, r, g, b); // Right
        AddQuad(m, p000, p001, p011, p010, r, g, b); // Left
        if (!openTop) AddQuad(m, p010, p011, p111, p110, r, g, b); // Top
        AddQuad(m, p000, p100, p101, p001, r, g, b); // Bottom

        return m;
    }

    mesh CreateCubeScales(vec3d scale, float r, float g, float b, bool openTop = false) {
        mesh composite;

        AddMesh(composite, CreateCube(0.5f, 0.5f, 0.5f, true), { 0,0,0 }, scale);
        return composite;
    }

    // PYRAMID (Base at 0, Point at 1)
    mesh CreatePyramid(float r, float g, float b)
    {
        mesh m;
        vec3d top = { 0.5f, 1.0f, 0.5f };
        vec3d fl = { 0,0,0 }; vec3d fr = { 1,0,0 };
        vec3d bl = { 0,0,1 }; vec3d br = { 1,0,1 };

        m.tris.push_back({ fl, top, fr, r, g, b });
        m.tris.push_back({ fr, top, br, r, g, b });
        m.tris.push_back({ br, top, bl, r, g, b });
        m.tris.push_back({ bl, top, fl, r, g, b });
        AddQuad(m, fl, fr, br, bl, r, g, b);
        return m;
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

                vec3d p1 = { x0, 0, z1 }; // Bottom Left
                vec3d p2 = { x1, 0, z1 }; // Bottom Right
                vec3d p3 = { x1, 0, z0 }; // Top Right
                vec3d p4 = { x0, 0, z0 }; // Top Left

                // Add small square
                AddQuad(m, p1, p2, p3, p4, r, g, b);
            }
        }
        return m;
    }
    mesh CreateFactoryUnit() {
        mesh composite;
        AddMesh(composite, CreateCube(0.5f, 0.5f, 0.5f, true), { 0,0,0 }, { 1,1,1 });

        // Sits on top (Y+ 1.0)
        AddMesh(composite, CreatePyramid(1.0f, 0.0f, 0.0f), { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
        return composite;
    }

    mesh CreateWarrior() {
        mesh composite;
        AddMesh(composite, CreateCube(0, 1, 0), { 0,0,0 }, { 0.5f, 0.8f, 0.5f });
        AddMesh(composite, CreateCube(1, 0.8f, 0.6f), { 0.1f, 0.8f, 0.1f }, { 0.3f, 0.3f, 0.3f });
        return composite;
    }

    mesh CreateEnemyWarrior() {
        mesh composite;
        AddMesh(composite, CreateCube(1, 0, 0), { 0,0,0 }, { 0.5f, 0.8f, 0.5f });
        AddMesh(composite, CreateCube(1, 0.8f, 0.6f), { 0.1f, 0.8f, 0.1f }, { 0.3f, 0.3f, 0.3f });
        return composite;
    }
}