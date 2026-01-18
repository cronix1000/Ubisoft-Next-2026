#include "ThreeDVisualiser.h"
struct Projectile {
    vec3d velocity;
    int damage;
    int ownerTeamId; // Don't hit your own team!
    float lifetime;  // Destroy after 2 seconds if it misses
};