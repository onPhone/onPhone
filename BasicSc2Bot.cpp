#include "BasicSc2Bot.h"
#include <iostream>
#include <sc2api/sc2_typeenums.h>
#include <sc2api/sc2_unit_filters.h>

using namespace sc2;

/**
 * @brief Initializes the build order for the Zerg bot.
 *
 * This function is called at the start of the game and sets up the initial
 * build order for the Zerg bot. It defines a sequence of actions to be
 * executed at specific supply counts, including:
 * - Building drones, overlords, and other structures
 * - Producing combat units like zerglings and roaches
 * - Researching upgrades
 */
void BasicSc2Bot::OnGameStart() {
    buildOrder.push({12, std::bind(&BasicSc2Bot::BuildDrone, this)});
    buildOrder.push({13, std::bind(&BasicSc2Bot::BuildOverlord, this)});
    buildOrder.push({16, std::bind(&BasicSc2Bot::BuildExtractor, this)});
    buildOrder.push({16, std::bind(&BasicSc2Bot::BuildSpawningPool, this)});
    buildOrder.push({17, std::bind(&BasicSc2Bot::BuildHatchery, this)});
    for (int i = 0; i < 6; ++i) {
        buildOrder.push({16, std::bind(&BasicSc2Bot::BuildZergling, this)});
    }
    buildOrder.push({19, std::bind(&BasicSc2Bot::BuildQueen, this)});
    buildOrder.push({21, std::bind(&BasicSc2Bot::BuildRoachWarren, this)});
    buildOrder.push(
        {21, std::bind(&BasicSc2Bot::ResearchMetabolicBoost, this)});
    buildOrder.push({21, std::bind(&BasicSc2Bot::BuildOverlord, this)});
    for (int i = 0; i < 4; ++i) {
        buildOrder.push({21, std::bind(&BasicSc2Bot::BuildRoach, this)});
    }
    buildOrder.push({29, std::bind(&BasicSc2Bot::BuildOverlord, this)});
    for (int i = 0; i < 10; ++i) {
        buildOrder.push({29, std::bind(&BasicSc2Bot::BuildZergling, this)});
    }
    buildOrder.push({34, std::bind(&BasicSc2Bot::BuildRavager, this)});
}

/**
 * @brief Executes the bot's logic on each game step.
 *
 * This function is called on each game step and is responsible for:
 * 1. Checking the current supply.
 * 2. Executing the next item in the build order if conditions are met.
 * 3. Building a Drone if no build order item was executed.
 *
 * The function prioritizes the build order but ensures continuous
 * worker production when the build order is empty or cannot be executed.
 */
void BasicSc2Bot::OnStep() {
    const ObservationInterface *observation = Observation();
    int currentSupply = observation->GetFoodWorkers();
    if (!buildOrder.empty()) {
        if (currentSupply >= buildOrder.front().first) {
            std::cout << buildOrder.front().first << std::endl;
            if (buildOrder.front().second()) {
                buildOrder.pop();
            }
        }
        Units units = observation->GetUnits(Unit::Alliance::Self);
        for (const auto &unit : units) {
            for (const auto &order : unit->orders) {
                if (order.ability_id == sc2::ABILITY_ID::TRAIN_DRONE) {
                    ++currentSupply;
                }
            }
        }
        if (currentSupply < buildOrder.front().first) {
            BuildDrone();
        }
    }
}

/**
 * @brief Attempts to build a Drone unit.
 *
 * This function checks for available larva and sufficient minerals,
 * then issues a command to train a Drone if conditions are met.
 *
 * @return true if a Drone was successfully queued for production, false
 * otherwise.
 */
bool BasicSc2Bot::BuildDrone() {
    const ObservationInterface *observation = Observation();
    Units larva = getIdleLarva();
    if (!larva.empty() && observation->GetMinerals() >= 50 &&
        observation->GetFoodUsed() < observation->GetFoodCap()) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_DRONE);
        return true;
    }
    return false;
}

/**
 * @brief Attempts to build an Overlord unit.
 *
 * This function checks if an Overlord has already been built, then looks for
 * available larva and sufficient minerals. If conditions are met, it issues a
 * command to train an Overlord.
 *
 * @return true if an Overlord was successfully queued for production or has
 * been built before, false otherwise.
 */
bool BasicSc2Bot::BuildOverlord() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units larva = getIdleLarva();
    if (!larva.empty() && observation->GetMinerals() >= 100) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_OVERLORD);
        built = true;
    }
    return built;
}

/**
 * @brief Attempts to build a Zergling unit.
 *
 * This function checks for available larva and sufficient minerals,
 * then issues a command to train a Zergling if conditions are met.
 * It uses a  flag to ensure only one Zergling is built.
 *
 * @return true if a Zergling was successfully queued for production or has been
 * built before, false otherwise.
 */
bool BasicSc2Bot::BuildZergling() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units larva = getIdleLarva();
    Units spawning_pool =
        getConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
    if (!larva.empty() && observation->GetMinerals() >= 25 &&
        observation->GetFoodUsed() < observation->GetFoodCap() &&
        !spawning_pool.empty()) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_ZERGLING);
        built = true;
    }
    return built;
}

/**
 * @brief Attempts to build a Queen unit.
 *
 * This function checks for a hatchery and spawning pool and sufficient
 * minerals, then issues a command to train a Queen if conditions are met. It
 * uses a  flag to ensure only one Queen is built.
 *
 * @return true if a Queen was successfully queued for production or has been
 * built before, false otherwise.
 */
bool BasicSc2Bot::BuildQueen() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units hatchery = getConstructedBuildings(sc2::UNIT_TYPEID::ZERG_HATCHERY);
    ;
    Units spawning_pool =
        getConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
    ;
    if (!hatchery.empty() && !spawning_pool.empty() &&
        observation->GetMinerals() >= 150 &&
        observation->GetFoodUsed() + 1 < observation->GetFoodCap()) {
        Actions()->UnitCommand(hatchery[0], ABILITY_ID::TRAIN_QUEEN);
        built = true;
    }
    return built;
}

/**
 * @brief Attempts to build a Roach unit.
 *
 * This function checks for available larva, sufficient minerals, and a roach
 * warren, then issues a command to train a Roach if conditions are met. It uses
 * a  flag to ensure only one Roach is built.
 *
 * @return true if a Roach was successfully queued for production or has been
 * built before, false otherwise.
 */
bool BasicSc2Bot::BuildRoach() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units larva = getIdleLarva();
    Units roach_warren =
        getConstructedBuildings(sc2::UNIT_TYPEID::ZERG_ROACHWARREN);
    ;
    if (!larva.empty() && observation->GetMinerals() >= 75 &&
        observation->GetVespene() >= 25 &&
        observation->GetFoodUsed() < observation->GetFoodCap() &&
        !roach_warren.empty()) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_ROACH);
        built = true;
    }
    return built;
}

/**
 * @brief Attempts to build a Ravager unit.
 *
 * This function checks for available roaches, sufficient minerals, and a roach
 * warren, then issues a command to train a Ravager if conditions are met. It
 * uses a  flag to ensure only one Roach is built.
 *
 * @return true if a Ravager was successfully queued for production or has been
 * built before, false otherwise.
 */
bool BasicSc2Bot::BuildRavager() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units roaches = observation->GetUnits(Unit::Alliance::Self,
                                          IsUnit(UNIT_TYPEID::ZERG_ROACH));
    Units roach_warren =
        getConstructedBuildings(sc2::UNIT_TYPEID::ZERG_ROACHWARREN);
    ;
    if (!roaches.empty() && observation->GetMinerals() >= 25 &&
        observation->GetVespene() >= 75 &&
        observation->GetFoodUsed() < observation->GetFoodCap() &&
        !roach_warren.empty()) {
        Actions()->UnitCommand(roaches[0], ABILITY_ID::MORPH_RAVAGER);
        built = true;
    }
    return built;
}

/**
 * @brief Attempts to build a Spawning Pool structure.
 *
 * This function checks if a Spawning Pool has already been built, then looks
 * for available drones and sufficient minerals. If conditions are met, it finds
 * a suitable location and issues a command to build a Spawning Pool.
 *
 * @return true if a Spawning Pool was successfully queued for construction or
 * has been built before, false otherwise.
 */
bool BasicSc2Bot::BuildSpawningPool() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units drones = getIdleWorkers();
    if (!drones.empty() && observation->GetMinerals() >= 200) {
        Point2D buildLocation =
            FindPlacementForBuilding(ABILITY_ID::BUILD_SPAWNINGPOOL);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            Units spawning_pool =
                getConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
            ;
            std::cout << "Spawning Pool built: " << spawning_pool.size()
                      << std::endl;
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_SPAWNINGPOOL,
                                   buildLocation);
            built = true;
            spawning_pool =
                getConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
            ;
            std::cout << "Spawning Pool built: " << spawning_pool.size()
                      << std::endl;
        }
    }
    return built;
}

/**
 * @brief Attempts to build an Extractor structure.
 * This function checks if an Extractor has already been built, then looks
 * for available drones and sufficient minerals. If conditions are met, it finds
 * a vespen geyser near the main Hatchery (within 15 units) and issues a command
 * to build an Extractor on it.
 *
 * @return true if an Extractor was successfully queued for construction or
 * has been built before, false otherwise.
 */
bool BasicSc2Bot::BuildExtractor() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units drones = getIdleWorkers();
    if (!drones.empty() && observation->GetMinerals() >= 25) {
        Units geysers =
            observation->GetUnits(Unit::Alliance::Neutral,
                                  IsUnit(UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        for (const auto &geyser : geysers) {
            float distance =
                Distance2D(geyser->pos, observation->GetStartLocation());
            if (distance < 15) {
                Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_EXTRACTOR,
                                       geyser);
                built = true;
                break;
            }
        }
    }
    return built;
}

bool BasicSc2Bot::BuildHatchery() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units drones = getIdleWorkers();
    if (!drones.empty() && observation->GetMinerals() >= 300) {
        Point2D buildLocation =
            FindPlacementForBuilding(ABILITY_ID::BUILD_HATCHERY);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_HATCHERY,
                                   buildLocation);
            built = true;
        }
    }

    return built;
}

bool BasicSc2Bot::BuildRoachWarren() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units drones = getIdleWorkers();
    if (!drones.empty() && observation->GetMinerals() >= 150) {
        Point2D buildLocation =
            FindPlacementForBuilding(ABILITY_ID::BUILD_ROACHWARREN);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_ROACHWARREN,
                                   buildLocation);
            built = true;
        }
    }

    return built;
}

/**
 * @brief Finds a suitable placement for a building near the main Hatchery.
 *
 * @param ability_type The ABILITY_ID of the building to be placed.
 * @return Point2D The coordinates where the building can be placed.
 *         Returns (0, 0) if no suitable location is found.
 */
Point2D BasicSc2Bot::FindPlacementForBuilding(ABILITY_ID ability_type) {
    const ObservationInterface *observation = Observation();
    Point2D hatchery_location;

    for (const auto &unit : observation->GetUnits(Unit::Alliance::Self)) {
        if (unit->unit_type == UNIT_TYPEID::ZERG_HATCHERY) {
            hatchery_location = unit->pos;
            break;
        }
    }

    float radius = 15.0f;
    for (float dx = -radius; dx <= radius; dx += 1.0f) {
        for (float dy = -radius; dy <= radius; dy += 1.0f) {
            Point2D buildLocation =
                Point2D(hatchery_location.x + dx, hatchery_location.y + dy);
            if (Query()->Placement(ability_type, buildLocation)) {
                return buildLocation;
            }
        }
    }

    return Point2D(0, 0);
}

Units BasicSc2Bot::getIdleWorkers() {
    Units idle_workers = Observation()->GetUnits(
        sc2::Unit::Alliance::Self, [](const sc2::Unit &unit) {
            // Check if the unit is a worker (SCV, Drone, or Probe)
            bool is_worker = unit.unit_type == sc2::UNIT_TYPEID::ZERG_DRONE;

            // Check if the worker has no orders or if its orders do not involve
            // gathering
            bool is_truly_idle =
                unit.orders.empty() ||
                unit.orders[0].ability_id == sc2::ABILITY_ID::HARVEST_GATHER ||
                unit.orders[0].ability_id == sc2::ABILITY_ID::HARVEST_RETURN;

            return is_worker && is_truly_idle;
        });
    return idle_workers;
}

bool BasicSc2Bot::ResearchMetabolicBoost() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units spawning_pool =
        getConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
    ;
    if (!spawning_pool.empty() && observation->GetMinerals() >= 100 &&
        observation->GetVespene() >= 100) {
        Actions()->UnitCommand(spawning_pool[0],
                               ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST);
        built = true;
    }
    return built;
}

Units BasicSc2Bot::getIdleLarva() {
    Units idle_larva = Observation()->GetUnits(
        sc2::Unit::Alliance::Self, [](const sc2::Unit &unit) {
            // Check if the unit is a worker (SCV, Drone, or Probe)
            bool is_larva = unit.unit_type == sc2::UNIT_TYPEID::ZERG_LARVA;

            // Check if the worker has no orders or if its orders do not involve
            // gathering
            bool is_truly_idle = unit.orders.empty();

            return is_larva && is_truly_idle;
        });
    return idle_larva;
}

Units BasicSc2Bot::getConstructedBuildings(UNIT_TYPEID type) {
    Units constructed_buildings = Observation()->GetUnits(
        sc2::Unit::Alliance::Self, [type](const sc2::Unit &unit) {
            return unit.build_progress == 1.0f && unit.unit_type == type;
        });

    return constructed_buildings;
}
