#pragma once

// game constants
#define EPSILON 0.000001
#define BASE_SIZE 15.0f
#define ENEMY_EPSILON 1.0f
#define CLUSTER_DISTANCE 20.0f
#define MAX_EXTRACTOR_WORKERS 3

// resource costs
#define DRONE_MINERAL_COST 50
#define DRONE_FOOD_COST 1
#define OVERLORD_MINERAL_COST 100
#define ZERGLING_MINERAL_COST 50
#define ZERGLING_FOOD_COST 1
#define QUEEN_MINERAL_COST 175
#define QUEEN_FOOD_COST 2
#define ROACH_MINERAL_COST 75
#define ROACH_VESPENE_COST 25
#define ROACH_FOOD_COST 2
#define RAVAGER_MINERAL_COST 25
#define RAVAGER_VESPENE_COST 75
#define SPAWNINGPOOL_COST 200
#define EXTRACTOR_COST 25
#define HATCHERY_COST 275
#define ROACHWARREN_COST 150
#define METABOLIC_BOOST_COST 100

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