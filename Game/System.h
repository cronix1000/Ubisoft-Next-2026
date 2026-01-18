#pragma once
#include <set>
#include "ECSBase.h" // Assuming this has your Entity ID typedefs

class System
{
public:
    // This Set automatically keeps track of every Entity 
    // that matches this System's requirements.
    std::set<Entity> mEntities;

    // Virtual destructor is crucial for inheritance
    virtual ~System() = default;
};