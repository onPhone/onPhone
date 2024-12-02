#ifndef ENUMS_H
#define ENUMS_H

enum class ROLE {
    SCOUT,
    ATTACK,
    WORKER,
    BUILDING,
    INTERMEDIATE // Refers to groups that become material for others, so drones that become buildings
};

enum class TASK {
    UNSET, // Specifically for group task setting
    ATTACK,
    MINE,
    EXTRACT,
    SCOUT,
    SCOUT_ALL,
    FAST_SCOUT,
    MOVE,
    RALLY
};

#endif // ENUMS_H