
#pragma once

#include "ECSBase.h"
struct GoldDepositComponent {
bool occupied = false;
int value = 1000;
Entity linkedFactory = static_cast<Entity>(-1);
};