#include "ECSBase.h"
struct OccupyingDepositComponent {
    Entity goldChunkEntity; // Pointer back to the resource we are sitting on
};