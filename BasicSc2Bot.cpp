#include "BasicSc2Bot.h"
#include <iostream>

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
    int currentSupply = observation->GetFoodUsed();

    bool buildOrderExecuted = false;
    if (!buildOrder.empty() && currentSupply >= buildOrder.front().first) {
        if (buildOrder.front().second()) {
            buildOrder.pop();
        }
        buildOrderExecuted = true;
    }

    if (!buildOrderExecuted) {
        BuildDrone();
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
    Units larva = observation->GetUnits(Unit::Alliance::Self,
                                        IsUnit(UNIT_TYPEID::ZERG_LARVA));
    if (!larva.empty() && observation->GetMinerals() >= 50) {
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
    static bool built = false;
    if (built) {
        return true;
    }
    const ObservationInterface *observation = Observation();
    Units larva = observation->GetUnits(Unit::Alliance::Self,
                                        IsUnit(UNIT_TYPEID::ZERG_LARVA));
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
 * It uses a static flag to ensure only one Zergling is built.
 *
 * @return true if a Zergling was successfully queued for production or has been
 * built before, false otherwise.
 */
bool BasicSc2Bot::BuildZergling() {
    static bool built = false;
    if (built) {
        return true;
    }
    const ObservationInterface *observation = Observation();
    Units larva = observation->GetUnits(Unit::Alliance::Self,
                                        IsUnit(UNIT_TYPEID::ZERG_LARVA));
    if (!larva.empty() && observation->GetMinerals() >= 100) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_OVERLORD);
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
    static bool built = false;
    if (built) {
        return true;
    }
    const ObservationInterface *observation = Observation();
    Units drones = observation->GetUnits(Unit::Alliance::Self,
                                         IsUnit(UNIT_TYPEID::ZERG_DRONE));
    if (!drones.empty() && observation->GetMinerals() >= 200) {
        Point2D buildLocation =
            FindPlacementForBuilding(ABILITY_ID::BUILD_SPAWNINGPOOL);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_SPAWNINGPOOL,
                                   buildLocation);
            built = true;
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
    static bool built = false;
    if (built) {
        return true;
    }
    const ObservationInterface *observation = Observation();
    Units drones = observation->GetUnits(Unit::Alliance::Self,
                                         IsUnit(UNIT_TYPEID::ZERG_DRONE));
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
    static bool built = false;
    if (built) {
        return true;
    }
    const ObservationInterface *observation = Observation();
    Units drones = observation->GetUnits(Unit::Alliance::Self,
                                         IsUnit(UNIT_TYPEID::ZERG_DRONE));
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

bool BasicSc2Bot::BuildQueen() { return true; }
bool BasicSc2Bot::BuildRoachWarren() { return true; }
bool BasicSc2Bot::ResearchMetabolicBoost() { return true; }
bool BasicSc2Bot::BuildRoach() { return true; }
bool BasicSc2Bot::BuildRavager() { return true; }