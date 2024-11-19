#include "BasicSc2Bot.h"
#include "cpp-sc2/include/sc2api/sc2_typeenums.h"
#include <iostream>
#include <limits>
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
    const Point2D startLoc = Observation()->GetStartLocation();
    const auto &gameInfo = Observation()->GetGameInfo();
    enemyLoc = Point2D(gameInfo.width - startLoc.x, gameInfo.height - startLoc.y);
    rallyPoint = Point2D((3 * enemyLoc.x + startLoc.x) / 4, (3 * enemyLoc.y + startLoc.y) / 4);

    constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)].push_back(
      Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY))[0]);
    buildOrder.push_back({12, std::bind(&BasicSc2Bot::BuildDrone, this)});
    buildOrder.push_back({13, std::bind(&BasicSc2Bot::BuildOverlord, this)});
    buildOrder.push_back({16, std::bind(&BasicSc2Bot::BuildExtractor, this)});
    buildOrder.push_back({16, std::bind(&BasicSc2Bot::BuildSpawningPool, this)});
    buildOrder.push_back({17, std::bind(&BasicSc2Bot::BuildHatchery, this)});
    for(int i = 0; i < 6; ++i) {
        buildOrder.push_back({16, std::bind(&BasicSc2Bot::BuildZergling, this)});
    }
    buildOrder.push_back({19, std::bind(&BasicSc2Bot::BuildQueen, this)});
    buildOrder.push_back({21, std::bind(&BasicSc2Bot::BuildRoachWarren, this)});
    buildOrder.push_back({21, std::bind(&BasicSc2Bot::ResearchMetabolicBoost, this)});
    buildOrder.push_back({21, std::bind(&BasicSc2Bot::BuildOverlord, this)});
    for(int i = 0; i < 4; ++i) {
        buildOrder.push_back({21, std::bind(&BasicSc2Bot::BuildRoach, this)});
    }
    buildOrder.push_back({29, std::bind(&BasicSc2Bot::BuildOverlord, this)});
    for(int i = 0; i < 10; ++i) {
        buildOrder.push_back({29, std::bind(&BasicSc2Bot::BuildZergling, this)});
    }
    buildOrder.push_back({34, std::bind(&BasicSc2Bot::BuildRavager, this)});
}

/**
 * @brief Executes the bot's main logic on each game step.
 *
 * This function is called on every game step and is responsible for
 * executing the current build order. It ensures that the bot continuously
 * progresses through its planned strategy by calling ExecuteBuildOrder().
 */
void BasicSc2Bot::OnStep() {
    ExecuteBuildOrder();
    if(!AttackMostDangerous()) { StartAttack(enemyLoc); }
}

/**
 * @brief Determines if a unit is a combat unit capable of attacking.
 *
 * This function checks if the given unit is one of the Zerg combat units
 * (Zergling, Roach, or Ravager) that can engage in combat.
 *
 * @param unit The unit to check
 * @return true if the unit is an attack unit, false otherwise
 */
bool IsAttackUnit(const Unit &unit) {
    switch(unit.unit_type.ToType()) {
    case UNIT_TYPEID::ZERG_ZERGLING: return true;
    case UNIT_TYPEID::ZERG_ROACH: return true;
    case UNIT_TYPEID::ZERG_RAVAGER: return true;
    default: return false;
    }
}

/**
 * @brief Determines if a unit can attack air units.
 *
 * This function checks if the given unit is one of the Zerg units
 * (Queen or Ravager) that can attack air units.
 *
 * @param unit The unit to check
 * @return true if the unit can attack air units, false otherwise
 */
bool CanAttackAir(const Unit &unit) {
    switch(unit.unit_type.ToType()) {
    case UNIT_TYPEID::ZERG_QUEEN: return true;
    case UNIT_TYPEID::ZERG_RAVAGER: return true;
    default: return false;
    }
}

/**
 * @brief Handles unit destruction events.
 *
 * This function is called whenever a unit is destroyed. It checks the type
 * of the destroyed unit and adds appropriate build orders to replace the lost unit.
 * Each unit type has a specific supply cost and build function associated with it.
 *
 * @param unit Pointer to the destroyed unit
 */
void BasicSc2Bot::OnUnitDestroyed(const Unit *unit) {
    switch(unit->unit_type.ToType()) {
    case UNIT_TYPEID::ZERG_ZERGLING:
        buildOrder.push_front({16, std::bind(&BasicSc2Bot::BuildZergling, this)});
        break;
    case UNIT_TYPEID::ZERG_ROACH:
        buildOrder.push_front({21, std::bind(&BasicSc2Bot::BuildRoach, this)});
        break;
    case UNIT_TYPEID::ZERG_RAVAGER:
        buildOrder.push_front({34, std::bind(&BasicSc2Bot::BuildRavager, this)});
        break;
    case UNIT_TYPEID::ZERG_QUEEN:
        buildOrder.push_front({19, std::bind(&BasicSc2Bot::BuildQueen, this)});
        break;
    case UNIT_TYPEID::ZERG_EXTRACTOR:
        buildOrder.push_front({16, std::bind(&BasicSc2Bot::BuildExtractor, this)});
        break;
    case UNIT_TYPEID::ZERG_HATCHERY:
        buildOrder.push_front({17, std::bind(&BasicSc2Bot::BuildHatchery, this)});
        break;
    case UNIT_TYPEID::ZERG_SPAWNINGPOOL:
        buildOrder.push_front({16, std::bind(&BasicSc2Bot::BuildSpawningPool, this)});
        break;
    default: return;
    }
}

/**
 * @brief Initiates an attack command for all attack units.
 *
 * This function commands all available attack units to move to and attack
 * the specified location, but only if there are no pending build orders.
 *
 * @param loc The target location for the attack
 */
void BasicSc2Bot::StartAttack(const Point2D &loc) {
    if(buildOrder.empty()) {
        isAttacking = true;
        const auto attack_units = Observation()->GetUnits(Unit::Alliance::Self, IsAttackUnit);
        Actions()->UnitCommand(attack_units, ABILITY_ID::ATTACK_ATTACK, loc);
    }
}

/**
 * @brief Initiates an attack on the most dangerous enemy unit.
 *
 * This function identifies and attacks the most dangerous enemy units based on their health and
 * shield. It separates units into two categories: those that can attack air units and those that
 * can only attack ground. Air-capable units will target the enemy unit with the lowest
 * health+shield, while ground units target the ground enemy with the lowest health+shield. The
 * attack is only initiated if there are no pending build orders.
 *
 * @return true if at least one enemy unit was targeted for attack, false otherwise
 */
bool BasicSc2Bot::AttackMostDangerous() {
    const Unit *most_dangerous_all = nullptr;
    const Unit *most_dangerous_ground = nullptr;
    if(buildOrder.empty()) {
        // currently attacks weakest enemy unit
        const auto enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
        float min_hp_all = std::numeric_limits<float>::max();
        float min_hp_ground = std::numeric_limits<float>::max();
        for(const auto &unit : enemy_units) {
            if(unit->health + unit->shield < min_hp_all) {
                min_hp_all = unit->health + unit->shield;
                most_dangerous_all = unit;
            }
            if(!unit->is_flying && unit->health + unit->shield < min_hp_ground) {
                min_hp_ground = unit->health + unit->shield;
                most_dangerous_ground = unit;
            }
        }
        if(most_dangerous_all) {
            const auto attack_units_all
              = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
                    return IsAttackUnit(unit) && CanAttackAir(unit);
                });
            Actions()->UnitCommand(attack_units_all, ABILITY_ID::ATTACK_ATTACK,
                                   most_dangerous_all->pos);
        }
        if(most_dangerous_ground) {
            const auto attack_units_ground
              = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
                    return IsAttackUnit(unit) && !CanAttackAir(unit);
                });
            Actions()->UnitCommand(attack_units_ground, ABILITY_ID::ATTACK_ATTACK,
                                   most_dangerous_ground->pos);
        }
    }
    return most_dangerous_all || most_dangerous_ground;
}

/**
 * @brief Handles unit creation events.
 *
 * This function is called whenever a new unit is created. It checks the type
 * of the created unit and performs specific actions based on the unit type.
 *
 * @param unit Pointer to the newly created unit.
 */
void BasicSc2Bot::OnUnitCreated(const Unit *unit) {
    switch(unit->unit_type.ToType()) {
    case UNIT_TYPEID::ZERG_ZERGLING:
    case UNIT_TYPEID::ZERG_ROACH:
    case UNIT_TYPEID::ZERG_RAVAGER: {
        if(isAttacking) {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, enemyLoc);
        } else {
            Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, rallyPoint);
        }
        break;
    }
    default: break;
    }
}

/**
 * @brief Handles building construction completion events.
 *
 * This function is called whenever a building finishes construction. It adds the
 * completed building to the appropriate tracking container and performs specific
 * actions based on the building type. For extractors, it automatically assigns
 * workers to harvest from them.
 *
 * @param unit Pointer to the newly constructed building.
 */
void BasicSc2Bot::OnBuildingConstructionComplete(const Unit *unit) {
    constructedBuildings[GetBuildingIndex(unit->unit_type)].push_back(unit);
    if(unit->unit_type == UNIT_TYPEID::ZERG_EXTRACTOR) { AssignWorkersToExtractor(unit); }
}

/**
 * @brief Executes the next item in the build order or builds a Drone.
 *
 * This function checks the current supply against the build order requirements.
 * If the supply meets or exceeds the next build order item's requirement,
 * it attempts to execute that item. If successful, the item is removed from
 * the queue. If the supply is insufficient for the next item, it attempts
 * to build a Drone instead.
 *
 * This method ensures continuous unit production by defaulting to Drone
 * construction when the build order cannot be followed.
 */
void BasicSc2Bot::ExecuteBuildOrder() {
    const ObservationInterface *observation = Observation();
    int currentSupply = observation->GetFoodUsed();

    if(!buildOrder.empty() && currentSupply >= buildOrder.front().first) {
        if(buildOrder.front().second()) { buildOrder.pop_front(); }
    } else if(!buildOrder.empty() && currentSupply < buildOrder.front().first) {
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
    Units larva = GetIdleLarva();
    if(!larva.empty() && observation->GetMinerals() >= 50
       && observation->GetFoodUsed() < observation->GetFoodCap()) {
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
    Units larva = GetIdleLarva();
    if(!larva.empty() && observation->GetMinerals() >= 100) {
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
    Units larva = GetIdleLarva();
    Units spawning_pool = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
    if(!larva.empty() && observation->GetMinerals() >= 25
       && observation->GetFoodUsed() < observation->GetFoodCap() && !spawning_pool.empty()) {
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
    Units hatchery = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)];
    Units spawning_pool = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
    if(!hatchery.empty() && !spawning_pool.empty() && observation->GetMinerals() >= 150
       && observation->GetFoodUsed() + 1 < observation->GetFoodCap()) {
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
    Units larva = GetIdleLarva();
    Units roach_warren = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_ROACHWARREN)];
    if(!larva.empty() && observation->GetMinerals() >= 75 && observation->GetVespene() >= 25
       && observation->GetFoodUsed() < observation->GetFoodCap() && !roach_warren.empty()) {
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
 * uses a  flag to ensure only one Ravager is built.
 *
 * @return true if a Ravager was successfully queued for production or has been
 * built before, false otherwise.
 */
bool BasicSc2Bot::BuildRavager() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units roaches = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_ROACH));
    Units roach_warren = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_ROACHWARREN)];
    if(!roaches.empty() && observation->GetMinerals() >= 25 && observation->GetVespene() >= 75
       && observation->GetFoodUsed() < observation->GetFoodCap() && !roach_warren.empty()) {
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
    Units drones = GetIdleWorkers();
    if(!drones.empty() && observation->GetMinerals() >= 200) {
        Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_SPAWNINGPOOL);
        if(buildLocation.x != 0 && buildLocation.y != 0) {
            Units spawning_pool
              = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_SPAWNINGPOOL, buildLocation);
            built = true;
        }
    }
    return built;
}

/**
 * @brief Attempts to build an Extractor structure.
 * This function checks if an Extractor has already been built, then looks
 * for available drones and sufficient minerals. If conditions are met, it finds
 * a vespene geyser near the main Hatchery (within 15 units) and issues a
 * command to build an Extractor on it.
 *
 * @return true if an Extractor was successfully queued for construction or
 * has been built before, false otherwise.
 */
bool BasicSc2Bot::BuildExtractor() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units drones = GetIdleWorkers();
    if(!drones.empty() && observation->GetMinerals() >= 25) {
        Units geysers = observation->GetUnits(Unit::Alliance::Neutral,
                                              IsUnit(UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        for(const auto &geyser : geysers) {
            float distance = Distance2D(geyser->pos, observation->GetStartLocation());
            if(distance < 15) {
                Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_EXTRACTOR, geyser);
                built = true;
                break;
            }
        }
    }
    return built;
}

/**
 * @brief Assigns workers to an Extractor.
 *
 * This function assigns up to 3 idle workers to the given Extractor.
 * It iterates through idle workers and commands them to work on the Extractor
 * using the SMART ability.
 *
 * @param extractor Pointer to the Extractor unit to assign workers to.
 */
void BasicSc2Bot::AssignWorkersToExtractor(const Unit *extractor) {
    Units workers = GetIdleWorkers();
    int assignedWorkers = 0;
    for(const auto &worker : workers) {
        if(assignedWorkers >= 3) break;
        Actions()->UnitCommand(worker, ABILITY_ID::SMART, extractor);
        assignedWorkers++;
    }
}

/**
 * @brief Attempts to build a Hatchery structure.
 *
 * This function checks for available idle drones and sufficient minerals.
 * If conditions are met, it finds a suitable location and issues a command
 * to build a Hatchery.
 *
 * @return bool Returns true if the build command was issued, false otherwise.
 */
bool BasicSc2Bot::BuildHatchery() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units drones = GetIdleWorkers();
    if(!drones.empty() && observation->GetMinerals() >= 300) {
        Point2D buildLocation = FindExpansionLocation();
        if(buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_HATCHERY, buildLocation);
            built = true;
        }
    }

    return built;
}

/**
 * @brief Builds a Roach Warren structure.
 *
 * This function attempts to build a Roach Warren if there's an idle Drone
 * and enough minerals available. It uses FindPlacementForBuilding to determine
 * a suitable location for the structure.
 *
 * @return bool Returns true if the build command was issued, false otherwise.
 */
bool BasicSc2Bot::BuildRoachWarren() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units drones = GetIdleWorkers();
    if(!drones.empty() && observation->GetMinerals() >= 150) {
        Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_ROACHWARREN);
        if(buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_ROACHWARREN, buildLocation);
            built = true;
        }
    }

    return built;
}

/**
 * @brief Researches Metabolic Boost upgrade for Zerglings.
 *
 * This function attempts to research the Metabolic Boost upgrade if there's a
 * constructed Spawning Pool and enough resources available.
 *
 * @return bool Returns true if the research command was issued, false
 * otherwise.
 */
bool BasicSc2Bot::ResearchMetabolicBoost() {
    bool built = false;

    const ObservationInterface *observation = Observation();
    Units spawning_pool = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
    if(!spawning_pool.empty() && observation->GetMinerals() >= 100
       && observation->GetVespene() >= 100) {
        Actions()->UnitCommand(spawning_pool[0], ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST);
        built = true;
    }
    return built;
}

/**
 * @brief Finds a suitable location for expanding the base.
 *
 * This function searches for mineral fields that are not too close to the
 * starting location and checks if there's enough space to build a Hatchery
 * nearby. It avoids locations where a Hatchery already exists.
 *
 * @return Point2D The coordinates where a new Hatchery can be placed for
 * expansion. Returns (0, 0) if no suitable location is found.
 */
Point2D BasicSc2Bot::FindExpansionLocation() {
    const ObservationInterface *observation = Observation();
    Units mineral_fields = observation->GetUnits();

    Point2D start_location = observation->GetStartLocation();
    float closest_distance = std::numeric_limits<float>::max();
    Point2D closest_location;

    // Minimum distance from starting location for new mineral field
    float min_distance_from_start = 32.0f;
    // Search radius around new mineral field for existing Hatcheries
    float existing_hatchery_search_radius = 14.0f;

    for(const auto &unit : mineral_fields) {
        if(unit->unit_type != UNIT_TYPEID::NEUTRAL_MINERALFIELD
           && unit->unit_type != UNIT_TYPEID::NEUTRAL_MINERALFIELD750) {
            continue;
        }

        float distance = Distance2D(unit->pos, start_location);

        if(distance < closest_distance && distance > min_distance_from_start) {
            Units nearby_units = observation->GetUnits(Unit::Alliance::Self, [&](const Unit &u) {
                return u.unit_type == UNIT_TYPEID::ZERG_HATCHERY
                       && Distance2D(u.pos, unit->pos) < existing_hatchery_search_radius;
            });

            if(nearby_units.empty()) {
                Point2D build_location = FindHatcheryPlacement(unit);
                if(build_location.x != 0 && build_location.y != 0) {
                    closest_distance = distance;
                    closest_location = build_location;
                }
            }
        }
    }
    return closest_location;
}

/**
 * @brief Finds a suitable placement for a Hatchery near a mineral field.
 *
 * This function searches for a suitable location to place a Hatchery near a
 * mineral field. It checks if the location is valid for building a Hatchery
 * and returns the coordinates if a suitable location is found.
 *
 * @param mineral_field Pointer to the mineral field unit.
 * @return Point2D The coordinates where the Hatchery can be placed.
 *         Returns (0, 0) if no suitable location is found.
 */
Point2D BasicSc2Bot::FindHatcheryPlacement(const Unit *mineral_field) {
    for(float curr_dx = 0; curr_dx <= 10; curr_dx += 1.0f) {
        for(float curr_dy = 0; curr_dy <= 10; curr_dy += 1.0f) {
            Point2D build_location(mineral_field->pos.x + curr_dx, mineral_field->pos.y + curr_dy);
            if(Query()->Placement(ABILITY_ID::BUILD_HATCHERY, build_location)) {
                return build_location;
            }
            if(curr_dy > 0) {
                build_location
                  = Point2D(mineral_field->pos.x + curr_dx, mineral_field->pos.y - curr_dy);
                if(Query()->Placement(ABILITY_ID::BUILD_HATCHERY, build_location)) {
                    return build_location;
                }
            }
        }
        if(curr_dx > 0) {
            for(float curr_dy = 0; curr_dy <= 10; curr_dy += 1.0f) {
                Point2D build_location(mineral_field->pos.x - curr_dx,
                                       mineral_field->pos.y + curr_dy);
                if(Query()->Placement(ABILITY_ID::BUILD_HATCHERY, build_location)) {
                    return build_location;
                }
                if(curr_dy > 0) {
                    build_location
                      = Point2D(mineral_field->pos.x - curr_dx, mineral_field->pos.y - curr_dy);
                    if(Query()->Placement(ABILITY_ID::BUILD_HATCHERY, build_location)) {
                        return build_location;
                    }
                }
            }
        }
    }
    return Point2D(0, 0);
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

    for(const auto &unit : observation->GetUnits(Unit::Alliance::Self)) {
        if(unit->unit_type == UNIT_TYPEID::ZERG_HATCHERY) {
            hatchery_location = unit->pos;
            break;
        }
    }

    float radius = 15.0f;
    for(float dx = -radius; dx <= radius; dx += 1.0f) {
        for(float dy = -radius; dy <= radius; dy += 1.0f) {
            Point2D buildLocation = Point2D(hatchery_location.x + dx, hatchery_location.y + dy);
            if(Query()->Placement(ability_type, buildLocation)) { return buildLocation; }
        }
    }
    return Point2D(0, 0);
}

/**
 * @brief Retrieves a list of idle worker units (drones).
 *
 * This function considers a worker as idle if it has no orders,
 * or if it's currently gathering resources or returning them.
 *
 * @return Units A collection of Unit objects representing idle workers.
 */
Units BasicSc2Bot::GetIdleWorkers() {
    return Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
        return unit.unit_type == UNIT_TYPEID::ZERG_DRONE
               && (unit.orders.empty() || unit.orders[0].ability_id == ABILITY_ID::HARVEST_GATHER
                   || unit.orders[0].ability_id == ABILITY_ID::HARVEST_RETURN);
    });
}

/**
 * @brief Retrieves a list of idle larva units.
 *
 * @return Units A collection of Unit objects representing idle larva.
 */
Units BasicSc2Bot::GetIdleLarva() {
    return Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
        return unit.unit_type == UNIT_TYPEID::ZERG_LARVA && unit.orders.empty();
    });
}

/**
 * @brief Retrieves the index of the building type in constructedBuildings.
 *
 * @param type The UNIT_TYPEID of the building to search for.
 * @return int The index of the building type in constructedBuildings.
 */
int BasicSc2Bot::GetBuildingIndex(UNIT_TYPEID type) {
    switch(type) {
    case UNIT_TYPEID::ZERG_HATCHERY: return 0;
    case UNIT_TYPEID::ZERG_EXTRACTOR: return 1;
    case UNIT_TYPEID::ZERG_SPAWNINGPOOL: return 2;
    case UNIT_TYPEID::ZERG_ROACHWARREN: return 3;
    default: return -1; // Invalid building type
    }
}
