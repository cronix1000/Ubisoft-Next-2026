#pragma once
#include "Components.h" // For vec3d

struct ParticleComponent
{
    vec3d velocity;
    float lifetime;      
    float maxLifetime;   
    float startScale;
    float endScale;
    float r, g, b;      
};