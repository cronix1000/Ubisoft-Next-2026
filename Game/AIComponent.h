#pragma once


struct AIComponent {
    enum class Type {
        None,
        ChasePlayer,
        Patrol,
        Flee
    };

    // Store it as an integer
    Type type = Type::None;
};
