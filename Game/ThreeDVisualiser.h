#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

using std::vector;
using std::string;
using std::ifstream;
using std::istringstream;
using std::max;

// 1. Data Structures
struct vec3d
{
    float x = 0;
    float y = 0;
    float z = 0;
    float w = 1;
};

struct triangle
{
    vec3d p[3];
    float r = 1.0f; // Red
    float g = 1.0f; // Green
    float b = 1.0f; // Blue
};

struct mesh
{
    vector<triangle> tris;

    bool LoadFromObjectFile(string sFilename)
    {
        ifstream f(sFilename);
        if (!f.is_open())
            return false;

        vector<vec3d> verts;

        while (!f.eof())
        {
            char line[128];
            f.getline(line, 128);

            std::stringstream s;
            s << line;

            char junk;

            if (line[0] == 'v')
            {
                vec3d v;
                s >> junk >> v.x >> v.y >> v.z;
                verts.push_back(v);
            }

            if (line[0] == 'f')
            {
                int f[3];
                s >> junk >> f[0] >> f[1] >> f[2];
                tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
            }
        }
        return true;
    }
};

struct mat4x4
{
    float m[4][4] = { 0 };
};

// 2. Math Library (Namespace to avoid class instantiation issues)
namespace Engine3D
{
    inline vec3d Vector_Add(vec3d& v1, vec3d& v2)
    {
        return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
    }

    inline vec3d Vector_Sub(vec3d& v1, vec3d& v2)
    {
        return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
    }

    inline vec3d Vector_Mul(vec3d& v1, float k)
    {
        return { v1.x * k, v1.y * k, v1.z * k };
    }

    inline vec3d Vector_Div(vec3d& v1, float k)
    {
        return { v1.x / k, v1.y / k, v1.z / k };
    }

    inline float Vector_DotProduct(vec3d& v1, vec3d& v2)
    {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    inline float Vector_Length(vec3d& v)
    {
        return sqrtf(Vector_DotProduct(v, v));
    }

    inline vec3d Vector_Normalise(vec3d& v)
    {
        float l = Vector_Length(v);
        return { v.x / l, v.y / l, v.z / l };
    }

    inline vec3d Vector_CrossProduct(vec3d& v1, vec3d& v2)
    {
        vec3d v;
        v.x = v1.y * v2.z - v1.z * v2.y;
        v.y = v1.z * v2.x - v1.x * v2.z;
        v.z = v1.x * v2.y - v1.y * v2.x;
        return v;
    }

    inline vec3d Matrix_MultiplyVector(mat4x4& m, vec3d& i)
    {
        vec3d v;
        v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
        v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
        v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
        v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];
        return v;
    }

    inline mat4x4 Matrix_MakeIdentity()
    {
        mat4x4 matrix;
        matrix.m[0][0] = 1.0f; matrix.m[1][1] = 1.0f; matrix.m[2][2] = 1.0f; matrix.m[3][3] = 1.0f;
        return matrix;
    }

    inline mat4x4 Matrix_MakeRotationX(float fAngleRad)
    {
        mat4x4 matrix = { 0 };
        matrix.m[0][0] = 1.0f;
        matrix.m[1][1] = cosf(fAngleRad);
        matrix.m[1][2] = sinf(fAngleRad);
        matrix.m[2][1] = -sinf(fAngleRad);
        matrix.m[2][2] = cosf(fAngleRad);
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    inline mat4x4 Matrix_MakeRotationY(float fAngleRad)
    {
        mat4x4 matrix = { 0 };
        matrix.m[0][0] = cosf(fAngleRad);
        matrix.m[0][2] = sinf(fAngleRad);
        matrix.m[2][0] = -sinf(fAngleRad);
        matrix.m[1][1] = 1.0f;
        matrix.m[2][2] = cosf(fAngleRad);
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    inline mat4x4 Matrix_MakeRotationZ(float fAngleRad)
    {
        mat4x4 matrix = { 0 };
        matrix.m[0][0] = cosf(fAngleRad);
        matrix.m[0][1] = sinf(fAngleRad);
        matrix.m[1][0] = -sinf(fAngleRad);
        matrix.m[1][1] = cosf(fAngleRad);
        matrix.m[2][2] = 1.0f;
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    inline mat4x4 Matrix_MakeTranslation(float x, float y, float z)
    {
        mat4x4 matrix = Matrix_MakeIdentity();
        matrix.m[3][0] = x;
        matrix.m[3][1] = y;
        matrix.m[3][2] = z;
        return matrix;
    }

    inline mat4x4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
    {
        float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
        mat4x4 matrix = { 0 };
        matrix.m[0][0] = fAspectRatio * fFovRad;
        matrix.m[1][1] = fFovRad;
        matrix.m[2][2] = fFar / (fFar - fNear);
        matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
        matrix.m[2][3] = 1.0f;
        matrix.m[3][3] = 0.0f;
        return matrix;
    }

    inline mat4x4 Matrix_MultiplyMatrix(mat4x4& m1, mat4x4& m2)
    {
        mat4x4 matrix = { 0 };
        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++)
                matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
        return matrix;
    }

    inline mat4x4 Matrix_PointAt(vec3d& pos, vec3d& target, vec3d& up)
    {
        vec3d newForward = Vector_Sub(target, pos);
        newForward = Vector_Normalise(newForward);

        vec3d a = Vector_Mul(newForward, Vector_DotProduct(up, newForward));
        vec3d newUp = Vector_Sub(up, a);
        newUp = Vector_Normalise(newUp);

        vec3d newRight = Vector_CrossProduct(newUp, newForward);

        mat4x4 matrix;
        matrix.m[0][0] = newRight.x;    matrix.m[0][1] = newRight.y;    matrix.m[0][2] = newRight.z;    matrix.m[0][3] = 0.0f;
        matrix.m[1][0] = newUp.x;       matrix.m[1][1] = newUp.y;       matrix.m[1][2] = newUp.z;       matrix.m[1][3] = 0.0f;
        matrix.m[2][0] = newForward.x;  matrix.m[2][1] = newForward.y;  matrix.m[2][2] = newForward.z;  matrix.m[2][3] = 0.0f;
        matrix.m[3][0] = pos.x;         matrix.m[3][1] = pos.y;         matrix.m[3][2] = pos.z;         matrix.m[3][3] = 1.0f;
        return matrix;
    }

    inline mat4x4 Matrix_MakeOrthographic(float width, float height, float fNear, float fFar)
    {
        mat4x4 matrix = { 0 };
        matrix.m[0][0] = 2.0f / width;
        matrix.m[1][1] = 2.0f / height;
        matrix.m[2][2] = 1.0f / (fFar - fNear);
        matrix.m[3][2] = -fNear / (fFar - fNear);
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    inline mat4x4 Matrix_QuickInverse(mat4x4& m)
    {
        mat4x4 matrix;
        matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
        matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
        matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
        matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
        matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
        matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
        matrix.m[3][3] = 1.0f;
        return matrix;
    }

    inline vec3d Vector_IntersectPlane(vec3d& plane_p, vec3d& plane_n, vec3d& lineStart, vec3d& lineEnd)
    {
        plane_n = Vector_Normalise(plane_n);
        float plane_d = -Vector_DotProduct(plane_n, plane_p);
        float ad = Vector_DotProduct(lineStart, plane_n);
        float bd = Vector_DotProduct(lineEnd, plane_n);
        float t = (-plane_d - ad) / (bd - ad);
        vec3d lineStartToEnd = Vector_Sub(lineEnd, lineStart);
        vec3d lineToIntersect = Vector_Mul(lineStartToEnd, t);
        return Vector_Add(lineStart, lineToIntersect);
    }

    inline mat4x4 Matrix_MakeOrthographic(float fLeft, float fRight, float fBottom, float fTop, float fNear, float fFar)
    {
        mat4x4 matrix = Matrix_MakeIdentity();
        matrix.m[0][0] = 2.0f / (fRight - fLeft);
        matrix.m[1][1] = 2.0f / (fTop - fBottom);
        matrix.m[2][2] = 2.0f / (fFar - fNear);
        matrix.m[3][0] = -(fRight + fLeft) / (fRight - fLeft);
        matrix.m[3][1] = -(fTop + fBottom) / (fTop - fBottom);
        matrix.m[3][2] = -(fFar + fNear) / (fFar - fNear);
        return matrix;
    }

    inline float Vector_Distance(vec3d posA, vec3d posB) {
        double dx = posB.x - posA.x;
        double dy = posB.y - posA.y;
        double dz = posB.z - posA.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    inline int Triangle_ClipAgainstPlane(vec3d plane_p, vec3d plane_n, triangle& in_tri, triangle& out_tri1, triangle& out_tri2)
    {
        plane_n = Vector_Normalise(plane_n);

        auto dist = [&](vec3d& p)
            {
                return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - Vector_DotProduct(plane_n, plane_p));
            };

        vec3d* inside_points[3];  int nInsidePointCount = 0;
        vec3d* outside_points[3]; int nOutsidePointCount = 0;

        float d0 = dist(in_tri.p[0]);
        float d1 = dist(in_tri.p[1]);
        float d2 = dist(in_tri.p[2]);

        if (d0 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[0]; }
        else { outside_points[nOutsidePointCount++] = &in_tri.p[0]; }
        if (d1 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[1]; }
        else { outside_points[nOutsidePointCount++] = &in_tri.p[1]; }
        if (d2 >= 0) { inside_points[nInsidePointCount++] = &in_tri.p[2]; }
        else { outside_points[nOutsidePointCount++] = &in_tri.p[2]; }

        if (nInsidePointCount == 0) return 0;

        if (nInsidePointCount == 3)
        {
            out_tri1 = in_tri;
            return 1;
        }

        if (nInsidePointCount == 1 && nOutsidePointCount == 2)
        {
            out_tri1.r = in_tri.r; out_tri1.g = in_tri.g; out_tri1.b = in_tri.b;
            out_tri1.p[0] = *inside_points[0];
            out_tri1.p[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
            out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1]);
            return 1;
        }

        if (nInsidePointCount == 2 && nOutsidePointCount == 1)
        {
            out_tri1.r = in_tri.r; out_tri1.g = in_tri.g; out_tri1.b = in_tri.b;
            out_tri2.r = in_tri.r; out_tri2.g = in_tri.g; out_tri2.b = in_tri.b;

            out_tri1.p[0] = *inside_points[0];
            out_tri1.p[1] = *inside_points[1];
            out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

            out_tri2.p[0] = *inside_points[1];
            out_tri2.p[1] = out_tri1.p[2];
            out_tri2.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0]);
            return 2;
        }

        return 0;
    }
}