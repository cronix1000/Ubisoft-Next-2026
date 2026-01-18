#pragma once
#include "Components.h"
#include <vector>

class ShapeFactory
{
public:
    // 1. Made Static: Call this anywhere without creating a class instance
    static mesh CreateCube()
    {
        // 2. Lazy Initialization: This code runs ONLY the first time this function is called.
        // The result is stored in 'meshCube' and reused forever after.
        static mesh meshCube = GenerateCubeMesh();
        return meshCube;
    }

    static mesh CreatePyramid()
    {
        static mesh meshPyramid = GeneratePyramidMesh();
        return meshPyramid;
    }

private:
    // Helper functions must be static to be called by static public functions
    static mesh GenerateCubeMesh() {
        mesh m;
        // SOUTH Face
        m.tris.push_back({ { {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f} }, 1.0f, 0.0f, 0.0f });
        m.tris.push_back({ {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} }, 1.0f, 0.0f, 0.0f });
        // NORTH Face
        m.tris.push_back({ { {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f} }, 0.0f, 0.0f, 1.0f });
        m.tris.push_back({ {{1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}, 0.0f, 0.0f, 1.0f });
        // EAST Face
        m.tris.push_back({ {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}, 0.0f, 1.0f, 0.0f });
        m.tris.push_back({ {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}}, 0.0f, 1.0f, 0.0f });
        // WEST Face
        m.tris.push_back({ {{0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}}, 1.0f, 1.0f, 0.0f });
        m.tris.push_back({ {{0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}, 1.0f, 1.0f, 0.0f });
        // TOP Face
        m.tris.push_back({ {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}, 0.0f, 1.0f, 1.0f });
        m.tris.push_back({ { {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}}, 0.0f, 1.0f, 1.0f });
        // BOTTOM Face
        m.tris.push_back({ {{1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}}, 1.0f, 0.0f, 1.0f });
        m.tris.push_back({ {{1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, 1.0f, 0.0f, 1.0f });
        return m;
    }

    static mesh GeneratePyramidMesh() {
        mesh m;
        // Base
        m.tris.push_back({ { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}}, 1.0f, 0.0f, 0.0f });
        m.tris.push_back({ {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}, 1.0f, 0.0f, 0.0f });
        // Sides (Meeting at 0.5, 1.0, 0.5)
        vec3d peak = { 0.5f, 1.0f, 0.5f };
        m.tris.push_back({ { {0.0f, 0.0f, 0.0f}, peak, {0.0f, 0.0f, 1.0f}}, 0.0f, 1.0f, 0.0f }); // West
        m.tris.push_back({ { {0.0f, 0.0f, 1.0f}, peak, {1.0f, 0.0f, 1.0f}}, 0.0f, 0.0f, 1.0f }); // South
        m.tris.push_back({ { {1.0f, 0.0f, 1.0f}, peak, {1.0f, 0.0f, 0.0f}}, 0.0f, 1.0f, 1.0f }); // East
        m.tris.push_back({ { {1.0f, 0.0f, 0.0f}, peak, {0.0f, 0.0f, 0.0f}}, 1.0f, 1.0f, 0.0f }); // North
        return m;
    }
};