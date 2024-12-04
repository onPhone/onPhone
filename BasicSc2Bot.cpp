#include "BasicSc2Bot.h"
#include "MasterController.h"

#include <cstddef>
#include <iostream>
#include <limits>

/**
 * @brief Gets the locations of enemy base if it is visible and sets enemyLoc.
 *
 * This function retrieves the locations of all enemy units that are either
 * visible or in snapshot and then establishes the enemy base location based
 * on whether they are buildings.
 *
 */
void BasicSc2Bot::GetEnemyUnitLocations() {
    Point2D scoutControllerEnemyLoc = this->controller.scout_controller.foundEnemyLocation;
    if(scoutControllerEnemyLoc.x != 0 && scoutControllerEnemyLoc.y != 0) {
        enemyLoc = scoutControllerEnemyLoc;
    } else {
        const ObservationInterface *observation = Observation();
        std::vector<Point3D> enemy_unit_locations;

        // Get all enemy units that are either visible or in snapshot
        Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy, [](const Unit &unit) {
            return unit.unit_type.ToType() != UNIT_TYPEID::INVALID &&  // Valid unit
                   (unit.display_type == Unit::DisplayType::Visible || // Visible or
                    unit.display_type == Unit::DisplayType::Snapshot)
                   && IsBuilding(unit);
        });
        if(!enemy_units.empty()) {
            if(enemyLoc != enemy_units[0]->pos) {
                enemyLoc = enemy_units[0]->pos;
                std::cout << "Enemy found at (" << enemyLoc.x << ", " << enemyLoc.y << ")\n";
            }
        }
    }
}

BasicSc2Bot::BasicSc2Bot() : controller(*this){};

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
    const auto &gameInfo = Observation()->GetGameInfo();
    startLoc = Observation()->GetStartLocation();
    std::cout << "Start location: (" << startLoc.x << ", " << startLoc.y << ")\n";
    mapCenter = (gameInfo.playable_min + gameInfo.playable_max) * 0.5f;
    std::cout << "Map center: (" << mapCenter.x << ", " << mapCenter.y << ")\n";
    top = startLoc.y > mapCenter.y;
    right = startLoc.x > mapCenter.x;
    std::size_t enemyLocationCount = Observation()->GetGameInfo().enemy_start_locations.size();
    if(enemyLocationCount == 1) {
        enemyLoc = Observation()->GetGameInfo().enemy_start_locations[0];
        this->controller.scout_controller.foundEnemyLocation = enemyLoc;
        enemyLocationCount = 0;
    }
    this->controller.addUnitGroup(UnitGroup(ROLE::INTERMEDIATE));
    this->controller.addUnitGroup(UnitGroup(ROLE::SCOUT, TASK::UNSET, enemyLocationCount));
    this->controller.addUnitGroup(UnitGroup(ROLE::ATTACK, TASK::RALLY));
    this->controller.addUnitGroup(UnitGroup(ROLE::WORKER));

    // Retrieve pointers to the corresponding UnitGroups
    this->Larva = &(this->controller.unitGroups[0]);
    this->Scouts = &(this->controller.unitGroups[1]);
    this->Attackers = &(this->controller.unitGroups[2]);
    this->Workers = &(this->controller.unitGroups[3]);

    constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)].push_back(
      Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY))[0]);
    buildOrder.push_back({13, std::bind(&BasicSc2Bot::BuildOverlord, this)});
    buildOrder.push_back({16, std::bind(&BasicSc2Bot::BuildExtractor, this)});
    buildOrder.push_back({16, std::bind(&BasicSc2Bot::BuildSpawningPool, this)});
    buildOrder.push_back({17, std::bind(&BasicSc2Bot::BuildHatchery, this)});
    for(int i = 0; i < 3; ++i) {
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
    for(int i = 0; i < 5; ++i) {
        buildOrder.push_back({29, std::bind(&BasicSc2Bot::BuildZergling, this)});
    }
    buildOrder.push_back({34, std::bind(&BasicSc2Bot::BuildRavager, this)});
    for(int i = 0; i < 5; ++i) {
        buildOrder.push_back({29, std::bind(&BasicSc2Bot::BuildZergling, this)});
    }
    buildOrder.push_back({19, std::bind(&BasicSc2Bot::BuildQueen, this)});
}

/**
 * @brief Executes the bot's main logic on each game step.
 *
 * This function is called on every game step and is responsible for
 * executing the current build order. It ensures that the bot continuously
 * progresses through its planned strategy by calling ExecuteBuildOrder().
 */
void BasicSc2Bot::OnStep() {
    GetEnemyUnitLocations();
    ExecuteBuildOrder();
    this->controller.step();
    // for(const auto &base : controller.scout_controller.base_locations) {
    //     Debug()->DebugSphereOut(base, 1.5f, Colors::Green);
    // }
    Debug()->SendDebug();
}

/**
 * @brief Tries to inject larvae into hatcheries using queens.
 *
 * This function checks if there are any available queens with enough energy
 * to inject larvae into hatcheries. If a queen is available, it is commanded
 * to inject larvae into the hatchery closest to it.
 */
void BasicSc2Bot::tryInjection() {
    const ObservationInterface *observation = Observation();

    Units queens = observation->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
        return unit.unit_type == UNIT_TYPEID::ZERG_QUEEN && unit.energy >= 25;
    });

    if(queens.empty()) { return; }

    Units hatcheries = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)];

    if(hatcheries.empty()) { return; }

    for(const auto &hatchery : hatcheries) {
        bool already_injected = false;
        for(const auto &order : hatchery->orders) {
            if(order.ability_id == ABILITY_ID::EFFECT_INJECTLARVA) {
                already_injected = true;
                break;
            }
        }

        if(!already_injected) {
            // Find the closest Queen to this Hatchery
            const Unit *closest_queen = nullptr;
            float closest_distance = std::numeric_limits<float>::max();

            for(const auto &queen : queens) {
                float distance = DistanceSquared2D(queen->pos, hatchery->pos);
                if(distance < closest_distance && queen->orders.empty()) {
                    closest_distance = distance;
                    closest_queen = queen;
                }
            }

            if(closest_queen) {
                Actions()->UnitCommand(closest_queen, ABILITY_ID::EFFECT_INJECTLARVA, hatchery);
                std::cout << "Command Sent: Injecting larvae into hatchery\n";
            }
        }
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
    if((unit->alliance == Unit::Alliance::Enemy)
       && (unit->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER
           || unit->unit_type == UNIT_TYPEID::PROTOSS_NEXUS
           || unit->unit_type == UNIT_TYPEID::ZERG_HATCHERY)
       && unit->pos.x == controller.scout_controller.foundEnemyLocation.x
       && unit->pos.y == controller.scout_controller.foundEnemyLocation.y) {
        controller.scout_controller.foundEnemyLocation.x = 0;
        controller.scout_controller.foundEnemyLocation.y = 0;
    } else if(unit->alliance != Unit::Alliance::Enemy) {
        switch(unit->unit_type.ToType()) {
        case UNIT_TYPEID::ZERG_ZERGLING:
            buildOrder.push_back({0, std::bind(&BasicSc2Bot::BuildZergling, this)});
            break;
        case UNIT_TYPEID::ZERG_ROACH:
            buildOrder.push_back({0, std::bind(&BasicSc2Bot::BuildRoach, this)});
            break;
        case UNIT_TYPEID::ZERG_RAVAGER:
            buildOrder.push_back({0, std::bind(&BasicSc2Bot::BuildRoach, this)});
            buildOrder.push_back({0, std::bind(&BasicSc2Bot::BuildRavager, this)});
            break;
        case UNIT_TYPEID::ZERG_QUEEN:
            buildOrder.push_back({0, std::bind(&BasicSc2Bot::BuildQueen, this)});
            break;
        case UNIT_TYPEID::ZERG_EXTRACTOR:
            OnBuildingDestruction(unit);
            buildOrder.push_front({0, std::bind(&BasicSc2Bot::BuildExtractor, this)});
            break;
        case UNIT_TYPEID::ZERG_HATCHERY:
            OnBuildingDestruction(unit);
            buildOrder.push_front({0, std::bind(&BasicSc2Bot::BuildHatchery, this)});
            break;
        case UNIT_TYPEID::ZERG_SPAWNINGPOOL:
            OnBuildingDestruction(unit);
            buildOrder.push_front({0, std::bind(&BasicSc2Bot::BuildSpawningPool, this)});
            break;
        default: break;
        }
    }
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
    case sc2::UNIT_TYPEID::ZERG_QUEEN: {
        this->Workers->addUnit(AllyUnit(unit, TASK::UNSET, this->Workers));
        break;
    }
    case UNIT_TYPEID::ZERG_DRONE: {
        this->Workers->addUnit(AllyUnit(unit, TASK::MINE, this->Workers));
        break;
    }
    case UNIT_TYPEID::ZERG_LARVA: {
        this->Larva->addUnit(AllyUnit(unit, this->Larva->unitTask, this->Larva));
        break;
    }
    case UNIT_TYPEID::ZERG_OVERLORD: {
        this->Scouts->addUnit(AllyUnit(unit, TASK::SCOUT, this->Scouts));
        break;
    }
    case UNIT_TYPEID::ZERG_ZERGLING:
        if(this->controller.scout_controller.zerglingCount < this->Scouts->sizeTrigger
           && enemyLoc.x == 0 && enemyLoc.y == 0) {
            this->Scouts->addUnit(AllyUnit(unit, TASK::FAST_SCOUT, this->Scouts));
            ++this->controller.scout_controller.zerglingCount;
            break;
        }
    case UNIT_TYPEID::ZERG_ROACH:
    case UNIT_TYPEID::ZERG_RAVAGER: {
        this->Attackers->addUnit(AllyUnit(unit, this->Attackers->unitTask, this->Attackers));
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
 * @brief Handles building destruction.
 *
 * This function is called whenever a building is destroyed. It removes the
 * destroyed building from the appropriate tracking container.
 *
 * @param unit Pointer to the destroyed building.
 */
void BasicSc2Bot::OnBuildingDestruction(const Unit *unit) {
    for(auto it = constructedBuildings[GetBuildingIndex(unit->unit_type)].begin();
        it != constructedBuildings[GetBuildingIndex(unit->unit_type)].end(); ++it) {
        if((*it)->tag == unit->tag) {
            constructedBuildings[GetBuildingIndex(unit->unit_type)].erase(it);
            return;
        }
    }
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
    const int currentSupply = Observation()->GetFoodUsed();
    const int maxSupply = Observation()->GetFoodCap();

    if(controller.attack_controller.isAttacking && currentSupply >= maxSupply - 4) {
        Units overlords_queued
          = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
                return (unit.unit_type == UNIT_TYPEID::ZERG_LARVA && !unit.orders.empty()
                        && unit.orders.front().ability_id == ABILITY_ID::TRAIN_OVERLORD)
                       || (unit.unit_type == UNIT_TYPEID::ZERG_EGG && !unit.orders.empty()
                           && unit.orders.front().ability_id == ABILITY_ID::TRAIN_OVERLORD);
            });

        if(overlords_queued.empty()) { BuildOverlord(); }
    }

    if(buildOrder.empty()) return;

    const auto &nextBuild = buildOrder.front();

    if(currentSupply >= nextBuild.first) {
        if(nextBuild.second()) { buildOrder.pop_front(); }
    } else {
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
    const int DRONE_MINERAL_COST = 50;
    const int DRONE_FOOD_COST = 1;

    if(observation->GetMinerals() < DRONE_MINERAL_COST
       || observation->GetFoodUsed() + DRONE_FOOD_COST > observation->GetFoodCap()) {
        return false;
    }

    Units larva = GetIdleLarva();
    if(larva.empty()) {
        tryInjection();
        return false;
    }

    Actions()->UnitCommand(larva.front(), ABILITY_ID::TRAIN_DRONE);
    std::cout << "Command Sent: Build Drone\n";
    return true;
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
    const ObservationInterface *observation = Observation();
    const int OVERLORD_MINERAL_COST = 100;

    if(observation->GetMinerals() < OVERLORD_MINERAL_COST) { return false; }

    Units larva = GetIdleLarva();
    if(larva.empty()) {
        tryInjection();
        return false;
    }

    Actions()->UnitCommand(larva.front(), ABILITY_ID::TRAIN_OVERLORD);
    std::cout << "Command Sent: Build Overlord\n";
    return true;
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
    const ObservationInterface *observation = Observation();
    const int ZERGLING_MINERAL_COST = 50;
    const int ZERGLING_FOOD_COST = 1;

    if(observation->GetMinerals() < ZERGLING_MINERAL_COST
       || observation->GetFoodUsed() + ZERGLING_FOOD_COST > observation->GetFoodCap()) {
        return false;
    }

    Units spawning_pool = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
    if(spawning_pool.empty()) { return false; }

    Units larva = GetIdleLarva();
    if(larva.empty()) {
        tryInjection();
        return false;
    }

    Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_ZERGLING);
    std::cout << "Command Sent: Build Zergling\n";
    return true;
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
    const ObservationInterface *observation = Observation();
    const int QUEEN_MINERAL_COST = 175;
    const int QUEEN_FOOD_COST = 2;

    if(observation->GetMinerals() < QUEEN_MINERAL_COST
       || observation->GetFoodUsed() + QUEEN_FOOD_COST > observation->GetFoodCap()) {
        return false;
    }

    Units hatchery = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)];
    Units spawning_pool = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];

    if(hatchery.empty() || spawning_pool.empty()) { return false; }

    Actions()->UnitCommand(hatchery[0], ABILITY_ID::TRAIN_QUEEN);
    std::cout << "Command Sent: Build Queen\n";
    return true;
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
    const ObservationInterface *observation = Observation();
    const int ROACH_MINERAL_COST = 75;
    const int ROACH_VESPENE_COST = 25;
    const int ROACH_FOOD_COST = 2;

    if(observation->GetMinerals() < ROACH_MINERAL_COST
       || observation->GetVespene() < ROACH_VESPENE_COST
       || observation->GetFoodUsed() + ROACH_FOOD_COST > observation->GetFoodCap()) {
        return false;
    }

    Units larva = GetIdleLarva();
    if(larva.empty()) {
        tryInjection();
        return false;
    }

    Units roach_warren = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_ROACHWARREN)];
    if(roach_warren.empty()) return false;

    Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_ROACH);
    std::cout << "Command Sent: Build Roach\n";
    return true;
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
    const ObservationInterface *observation = Observation();
    const int RAVAGER_MINERAL_COST = 25;
    const int RAVAGER_VESPENE_COST = 75;

    if(observation->GetMinerals() < RAVAGER_MINERAL_COST
       || observation->GetVespene() < RAVAGER_VESPENE_COST
       || observation->GetFoodUsed() >= observation->GetFoodCap()) {
        return false;
    }

    Units roach_warren = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_ROACHWARREN)];
    if(roach_warren.empty()) return false;

    Units roaches = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_ROACH));
    if(roaches.empty()) return false;

    Actions()->UnitCommand(roaches[0], ABILITY_ID::MORPH_RAVAGER);
    std::cout << "Command Sent: Build Ravager\n";
    return true;
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
    const ObservationInterface *observation = Observation();
    const int SPAWNINGPOOL_COST = 200;
    if(observation->GetMinerals() < SPAWNINGPOOL_COST) return false;

    AllyUnit *drone = nullptr;
    for(auto &worker : this->Workers->units) {
        if(worker.unitTask == TASK::MINE) {
            drone = &worker;
            break;
        }
    }
    if(!drone) return false;

    Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_SPAWNINGPOOL);
    if(buildLocation.x == 0 && buildLocation.y == 0) return false;

    drone->unitTask = TASK::UNSET;
    Actions()->UnitCommand(drone->unit, ABILITY_ID::BUILD_SPAWNINGPOOL, buildLocation);
    std::cout << "Command Sent: Build Spawning Pool at (" << buildLocation.x << ", "
              << buildLocation.y << ")\n";
    return true;
}

/**
 * @brief Checks if a unit is a type of vespene geyser
 * @param unit The unit to check
 * @return true if the unit is a vespene geyser, false otherwise
 */
bool BasicSc2Bot::IsGeyser(const Unit &unit) {
    switch(unit.unit_type.ToType()) {
    case UNIT_TYPEID::NEUTRAL_VESPENEGEYSER:
    case UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER:
    case UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER:
    case UNIT_TYPEID::NEUTRAL_PURIFIERVESPENEGEYSER:
    case UNIT_TYPEID::NEUTRAL_SHAKURASVESPENEGEYSER:
    case UNIT_TYPEID::NEUTRAL_RICHVESPENEGEYSER: return true;
    default: return false;
    }
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
    const ObservationInterface *observation = Observation();
    const int EXTRACTOR_COST = 25;
    if(observation->GetMinerals() < EXTRACTOR_COST) return false;

    AllyUnit *drone = nullptr;
    for(auto &worker : this->Workers->units) {
        if(worker.unitTask == TASK::MINE) {
            drone = &worker;
            break;
        }
    }
    if(!drone) return false;

    Units geysers = observation->GetUnits(Unit::Alliance::Neutral,
                                          [this](const Unit &unit) { return IsGeyser(unit); });
    Point2D startLocation;
    if(constructedBuildings[GetBuildingIndex(sc2::UNIT_TYPEID::ZERG_HATCHERY)].size() > 0) {
        startLocation
          = constructedBuildings[GetBuildingIndex(sc2::UNIT_TYPEID::ZERG_HATCHERY)][0]->pos;
    } else {
        startLocation = startLoc;
    }

    for(const auto &geyser : geysers) {
        if(Distance2D(geyser->pos, startLocation) < BASE_SIZE) {
            drone->unitTask = TASK::UNSET;
            Actions()->UnitCommand(drone->unit, ABILITY_ID::BUILD_EXTRACTOR, geyser);
            std::cout << "Command Sent: Build Extractor\n";
            return true;
        }
    }
    return false;
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
    int assignedWorkers = 0;
    int maxWorkers = 3;
    for(auto &worker : this->Workers->units) {
        if(assignedWorkers >= maxWorkers) break;
        if(worker.unitTask == TASK::MINE) {
            ++assignedWorkers;
            worker.unitTask = TASK::EXTRACT;
        }
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
    const ObservationInterface *observation = Observation();
    const int HATCHERY_COST = 275;
    if(observation->GetMinerals() < HATCHERY_COST) return false;

    AllyUnit *drone = nullptr;
    for(auto &worker : this->Workers->units) {
        if(worker.unitTask == TASK::MINE) {
            drone = &worker;
            break;
        }
    }

    if(!drone) return false;

    Point2D buildLocation = FindExpansionLocation();
    if(buildLocation.x == 0 && buildLocation.y == 0) return false;

    drone->unitTask = TASK::UNSET;
    Actions()->UnitCommand(drone->unit, ABILITY_ID::BUILD_HATCHERY, buildLocation);
    std::cout << "Command Sent: Build Hatchery at (" << buildLocation.x << ", " << buildLocation.y
              << ")\n";
    return true;
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
    const ObservationInterface *observation = Observation();
    const int ROACHWARREN_COST = 150;
    if(observation->GetMinerals() < ROACHWARREN_COST) return false;

    AllyUnit *drone = nullptr;
    for(auto &worker : this->Workers->units) {
        if(worker.unitTask == TASK::MINE) {
            drone = &worker;
            break;
        }
    }
    if(!drone) return false;

    Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_ROACHWARREN);
    if(buildLocation.x == 0 && buildLocation.y == 0) return false;

    drone->unitTask = TASK::UNSET;
    Actions()->UnitCommand(drone->unit, ABILITY_ID::BUILD_ROACHWARREN, buildLocation);
    std::cout << "Command Sent: Build Roach Warren\n";
    return true;
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
    const ObservationInterface *observation = Observation();
    const int METABOLIC_BOOST_COST = 100;
    const auto &spawning_pool
      = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];

    if(spawning_pool.empty() || observation->GetMinerals() < METABOLIC_BOOST_COST
       || observation->GetVespene() < METABOLIC_BOOST_COST) {
        return !spawning_pool.empty() && !spawning_pool[0]->orders.empty()
               && spawning_pool[0]->orders[0].ability_id
                    == ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST;
    }

    const Unit *pool = spawning_pool[0];
    Actions()->UnitCommand(pool, ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST);
    std::cout << "Command Sent: Research Metabolic Boost\n";
    return false;
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
    Point2D startLocation;
    if(constructedBuildings[GetBuildingIndex(sc2::UNIT_TYPEID::ZERG_HATCHERY)].size() > 0) {
        startLocation
          = constructedBuildings[GetBuildingIndex(sc2::UNIT_TYPEID::ZERG_HATCHERY)][0]->pos;
    } else {
        startLocation = startLoc;
    }
    Units minerals = observation->GetUnits(Unit::Alliance::Neutral, [](const Unit &u) {
        return u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD
               || u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750;
    });

    std::sort(minerals.begin(), minerals.end(), [startLocation](const Unit *a, const Unit *b) {
        return Distance2D(a->pos, startLocation) < Distance2D(b->pos, startLocation);
    });

    auto it = std::find_if(minerals.begin(), minerals.end(), [startLocation](const Unit *m) {
        return Distance2D(m->pos, startLocation) > 10.0f;
    });

    while(it != minerals.end()) {
        const Unit *mineral = *it;

        Units nearby = observation->GetUnits(Unit::Alliance::Self, [&mineral](const Unit &u) {
            return u.unit_type == UNIT_TYPEID::ZERG_HATCHERY
                   && Distance2D(u.pos, mineral->pos) < 10.0f;
        });

        if(nearby.empty()) {
            Point2D location = FindHatcheryPlacement(mineral);
            if(location.x != 0 || location.y != 0) { return location; }
        }
        ++it;
    }

    return Point2D(0, 0);
}

/**
 * @brief Finds a suitable location to place a Hatchery near a mineral field.
 *
 * @param mineral_field Pointer to a mineral field Unit to build the Hatchery near
 * @return Point2D The coordinates where the Hatchery can be placed, or (0,0) if no valid location
 * found
 */
Point2D BasicSc2Bot::FindHatcheryPlacement(const Unit *mineral_field) {
    const Point2D mineral_pos = mineral_field->pos;
    const Point2D likely_offsets[] = {Point2D(7, 0), Point2D(-7, 0), Point2D(0, 7), Point2D(0, -7)};

    for(const auto &offset : likely_offsets) {
        Point2D pos(mineral_pos.x + offset.x, mineral_pos.y + offset.y);
        if(Query()->Placement(ABILITY_ID::BUILD_HATCHERY, pos)) { return pos; }
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
    static int dx[] = {1, 0, -1, 0};
    static int dy[] = {0, 1, 0, -1};
    if(right) {
        dx[0] = -1;
        dx[2] = 1;
    }
    if(top) {
        dy[1] = -1;
        dy[3] = 1;
    }
    Point2D current;
    if(constructedBuildings[GetBuildingIndex(sc2::UNIT_TYPEID::ZERG_HATCHERY)].size() > 0) {
        current = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)][0]->pos;
    } else {
        current = Observation()->GetStartLocation();
    }

    for(int radius = 1; radius <= 15; ++radius) {
        for(int dir = 0; dir < 4; ++dir) {
            const int steps = radius;
            const float delta_x = dx[dir];
            const float delta_y = dy[dir];

            for(int step = 0; step < steps; ++step) {
                if(Query()->Placement(ability_type, current)) { return current; }
                current.x += delta_x;
                current.y += delta_y;
            }
        }
    }

    return Point2D(0, 0);
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

void BasicSc2Bot::OnGameEnd() {
    const ObservationInterface *observation = Observation();
    std::cout << "Game ended after: " << observation->GetGameLoop() << " loops " << std::endl;
    std::cout << "Total game time: " << observation->GetGameLoop() / 22.4 << " seconds"
              << std::endl;

    const Score &score = observation->GetScore();
    const std::vector<PlayerResult> result = observation->GetResults();
    std::cout << "Result: "
              << (result[0].result == GameResult::Win    ? "Won"
                  : result[0].result == GameResult::Loss ? "Lost"
                                                         : "Tied")
              << std::endl;
}