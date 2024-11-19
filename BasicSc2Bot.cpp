#include "BasicSc2Bot.h"
#include "cpp-sc2/include/sc2api/sc2_typeenums.h"
#include <iostream>
#include <limits>
#include <sc2api/sc2_typeenums.h>
#include <sc2api/sc2_unit_filters.h>
#include <cstdlib>

using namespace sc2;

#define EPSILON 0.000001
#define BASE_SIZE 30

bool AllyUnit::isMoving() const {
    if (this->unit == nullptr) {
        return false;
    }

    if (abs(priorPos.x-unit->pos.x) > EPSILON && abs(priorPos.y-unit->pos.y) > EPSILON) {
        return true;
    } else {
        return false;
    }
}

AllyUnit::AllyUnit(const sc2::Unit* unit, TASK task = TASK::UNSET, UnitGroup* group = nullptr) {
    this->unit = unit;
    this->unitTask = task;
    this->priorHealth = unit->health;
    this->group = group;
};

bool AllyUnit::underAttack() const {
    if (this->unit == nullptr) {
        return false;
    }
    return this->unit->health < priorHealth;
};


UnitController::UnitController(BasicSc2Bot* bot) : bot(bot) {};

void UnitController::underAttack(AllyUnit& unit) {

};

void UnitController::base_step(AllyUnit& unit) {
    if (unit.underAttack()) {
        underAttack(unit);
    }
    else {
        step(unit);
    }
};

ScoutController::ScoutController(BasicSc2Bot* bot) : UnitController(bot) {};

void ScoutController::step(AllyUnit& unit) {
    switch (unit.unitTask) {
        case TASK::SCOUT:
            scout(unit);
            break;
        case TASK::SCOUT_ALL:
            scout_all(unit);
            break;
        default:
            break;
    }
};

void ScoutController::scout(AllyUnit& unit) { 
    if (unit.unit != nullptr && !unit.isMoving()) {
        unit.group->index = (unit.group->index + 1)%bot->baseLocations.size();
        bot->Actions()->UnitCommand(unit.unit, sc2::ABILITY_ID::SMART, bot->baseLocations[unit.group->index]);
    }
};

void ScoutController::scout_all(AllyUnit& unit) { 
    if (unit.unit != nullptr && !unit.isMoving()) {
        unit.group->index = (unit.group->index + 1)%bot->waypoints.size();
        bot->Actions()->UnitCommand(unit.unit, sc2::ABILITY_ID::SMART, bot->waypoints[unit.group->index]);
    }
};

void ScoutController::underAttack(AllyUnit& unit) {

};

void ScoutController::onDeath(AllyUnit& unit) {

};


WorkerController::WorkerController(BasicSc2Bot* bot) : UnitController(bot) {};

void WorkerController::step(AllyUnit& unit) {
    switch (unit.unitTask) {
        case TASK::EXTRACT:
            extract(unit);
            break;
        case TASK::MINE:
            mine(unit);
            break;
        default:
            break;
    }
};

void WorkerController::extract(AllyUnit& unit) {
    std::cout << "SHOULD EXTRACT\n";
    bool is_extracting = false;
    for (const auto& order : unit.unit->orders) {
        if (order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER) {
                const sc2::Unit* target = bot->Observation()->GetUnit(order.target_unit_tag);
                if (target != nullptr && target->unit_type == sc2::UNIT_TYPEID::ZERG_EXTRACTOR) {
                    is_extracting = true;
                }
            break;
        } else if (order.ability_id == sc2::ABILITY_ID::HARVEST_RETURN) {
            is_extracting = true;
            break;
        }
    }
    if (!is_extracting) {
        auto extractors = bot->Observation()->GetUnits(sc2::Unit::Alliance::Self, [](const sc2::Unit &unit) {
            return 
                unit.unit_type == sc2::UNIT_TYPEID::ZERG_EXTRACTOR &&
                unit.assigned_harvesters < unit.ideal_harvesters;
            });
        if (!extractors.empty()) {
            sc2::Point3D starting_base = bot->Observation()->GetStartLocation();
            std::sort(extractors.begin(), extractors.end(), [&starting_base](const sc2::Unit* a, const sc2::Unit* b) {
                return DistanceSquared2D(a->pos, starting_base) < DistanceSquared2D(b->pos, starting_base);
            });
            bot->Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART, extractors[0]);
        }
    }
};

void WorkerController::mine(AllyUnit& unit) {
    bool is_extracting = false;
    for (const auto& order : unit.unit->orders) {
        if (order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER) {
                const sc2::Unit* target = bot->Observation()->GetUnit(order.target_unit_tag);
                if (target != nullptr && (target->unit_type == sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD ||
                    target->unit_type == sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
                    target->unit_type == sc2::UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD ||
                    target->unit_type == sc2::UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750)) {
                        is_extracting = true;
                }
            break;
        }
        else if (order.ability_id == sc2::ABILITY_ID::HARVEST_RETURN) {
            is_extracting = true;
            break;
        }
    }
    if (!is_extracting) {
        auto minerals = bot->Observation()->GetUnits(sc2::Unit::Alliance::Neutral, [](const sc2::Unit& unit) {
            return 
                (unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD ||
                 unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD750) &&
                unit.mineral_contents != 0;
        });

        if (!minerals.empty()) {
            sc2::Point3D starting_base = bot->Observation()->GetStartLocation();

            std::sort(minerals.begin(), minerals.end(), [&starting_base](const sc2::Unit* a, const sc2::Unit* b) {
                return DistanceSquared2D(a->pos, starting_base) < DistanceSquared2D(b->pos, starting_base);
            });

            for (const auto* mineral : minerals) {
                const auto& workers = bot->Observation()->GetUnits(sc2::Unit::Alliance::Self, [](const sc2::Unit& unit) {
                    return unit.unit_type == sc2::UNIT_TYPEID::ZERG_DRONE;
                });

                for (const auto* worker : workers) {
                    for (const auto& order : worker->orders) {
                        if (order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER && 
                            order.target_unit_tag == mineral->tag) {
                            bot->Actions()->UnitCommand(unit.unit, sc2::ABILITY_ID::SMART, mineral);
                            return;
                        }
                    }
                }
            }
        }
    }
};

void WorkerController::underAttack(AllyUnit& unit) {

};

void WorkerController::onDeath(AllyUnit& unit) {

};


AttackController::AttackController(BasicSc2Bot* bot) : UnitController(bot) {};

void AttackController::step(AllyUnit& unit) {
    switch (unit.unitTask) {
        case TASK::ATTACK:
            break;
        case TASK::RALLY:
            break;
        default:
            break;
    }
};

void AttackController::underAttack(AllyUnit& unit) {

};

void AttackController::onDeath(AllyUnit& unit) {

};

MasterController::MasterController(BasicSc2Bot* bot) : worker_controller(bot), scout_controller(bot), attack_controller(bot) {};

void MasterController::addUnitGroup(UnitGroup unit) {
    unitGroups.push_back(unit);
}

UnitGroup::UnitGroup(ROLE unitRole, TASK unitTask = TASK::UNSET, int sizeTrigger = 0) : unitRole(unitRole), unitTask(unitTask), sizeTrigger(sizeTrigger) {};

void UnitGroup::addUnit(AllyUnit unit) {
    this->units.push_back(unit);
    if (this->unitTask != TASK::UNSET) {
        unit.unitTask = this->unitTask;
    }
}

void MasterController::step() {
    for (auto& unitGroup : this->unitGroups) {
        std::vector<AllyUnit> new_units;
        for (auto& unit : unitGroup.units) {
            if (unit.unit != nullptr && unit.unit->is_alive && unit.unit->health > 0) {
                switch (unitGroup.unitRole) {
                    case ROLE::SCOUT:
                        scout_controller.base_step(unit);
                        break;
                    case ROLE::ATTACK:
                        attack_controller.base_step(unit);
                        break;
                    case ROLE::WORKER:
                        worker_controller.base_step(unit);
                        break;
                    default:
                        break;
                }
                unit.priorHealth = unit.unit->health;
                unit.priorPos = unit.unit->pos;
                if (unitGroup.unitTask != TASK::UNSET) {
                    unit.unitTask = unitGroup.unitTask; // Done this way so if we want to override group tasks, currently temporary
                }
                new_units.push_back(unit);
            } else {
                unit.unit = nullptr;
                switch (unitGroup.unitRole) {
                    case ROLE::SCOUT:
                        scout_controller.onDeath(unit);
                        break;
                    case ROLE::ATTACK:
                        attack_controller.onDeath(unit);
                        break;
                    case ROLE::WORKER:
                        worker_controller.onDeath(unit);
                        break;
                    default:
                        break;
                }
            }
        }
        unitGroup.units = new_units;
    }
};

BasicSc2Bot::BasicSc2Bot() : controller(this) {};

void BasicSc2Bot::initializeWaypoints() {
    const sc2::GameInfo& game_info = Observation()->GetGameInfo();
    sc2::Point2D map_center = (game_info.playable_min + game_info.playable_max) * 0.5f;
    std::cout << game_info.playable_min.x << " " << game_info.playable_min.y << "\n";
    std::cout << game_info.playable_max.x << " " << game_info.playable_max.y << "\n";
    const float step_size = 15.0f; // Distance between waypoints
    for (float x = game_info.playable_min.x; x < game_info.playable_max.x; x += step_size) {
        for (float y = game_info.playable_min.y; y < game_info.playable_max.y; y += step_size) {
            sc2::Point2D waypoint(x, y);
            // Check if the waypoint is reachable
            if (Query()->PathingDistance(map_center, waypoint) > 0) {
                waypoints.push_back(waypoint);
            }
        }
    }
};

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
    initializeWaypoints();
    initializeBaseLocations();
    // Create and move UnitGroups into the MasterController
    this->controller.addUnitGroup(UnitGroup(ROLE::INTERMEDIATE));
    this->controller.addUnitGroup(UnitGroup(ROLE::BUILDING));
    this->controller.addUnitGroup(UnitGroup(ROLE::WORKER, TASK::MINE));
    this->controller.addUnitGroup(UnitGroup(ROLE::WORKER, TASK::EXTRACT));
    this->controller.addUnitGroup(UnitGroup(ROLE::SCOUT, TASK::SCOUT));
    this->controller.addUnitGroup(UnitGroup(ROLE::ATTACK, TASK::RALLY, 12));
    this->controller.addUnitGroup(UnitGroup(ROLE::SCOUT, TASK::SCOUT));

    // Retrieve pointers to the corresponding UnitGroups
    this->Intermediates = &(this->controller.unitGroups[0]);
    this->Buildings = &(this->controller.unitGroups[1]);
    UnitGroup* InitialMiners = &(this->controller.unitGroups[2]);
    UnitGroup* InitialExtractors = &(this->controller.unitGroups[3]);
    this->Scouts = &(this->controller.unitGroups[4]);
    UnitGroup* InitialAttack = &(this->controller.unitGroups[5]);

    auto drones = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
        return unit.unit_type == UNIT_TYPEID::ZERG_DRONE;
    });
    for (const Unit* &unit : drones) {
        InitialMiners->addUnit(AllyUnit(unit, InitialMiners->unitTask, InitialMiners));
    }

    auto overlords = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
        return unit.unit_type == UNIT_TYPEID::ZERG_OVERLORD;
    });
    for (const Unit* &unit : overlords) {
        this->Scouts->addUnit(AllyUnit(unit, this->Scouts->unitTask, this->Scouts));
    }

    auto buildings = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
        return unit.unit_type == UNIT_TYPEID::ZERG_HATCHERY;
    });
    for (const Unit* &unit : buildings) {
        InitialMiners->addUnit(AllyUnit(unit, this->Buildings->unitTask, this->Buildings));
    }

    for (int i = 0; i < 4; ++i) {
        build_priorities.push({100, {sc2::UNIT_TYPEID::ZERG_DRONE, InitialMiners}});
    }
    build_priorities.push({97, {sc2::UNIT_TYPEID::ZERG_EXTRACTOR, this->Buildings}});
    for (int i = 0; i < 3; ++i) {
        build_priorities.push({96, {sc2::UNIT_TYPEID::ZERG_DRONE, InitialExtractors}});
    }
    build_priorities.push({95, {sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, this->Buildings}});
    build_priorities.push({94, {sc2::UNIT_TYPEID::ZERG_HATCHERY, this->Buildings}});
    build_priorities.push({93, {sc2::UNIT_TYPEID::ZERG_QUEEN, InitialAttack}});
    for (int i = 0; i < 6; ++i) {
        build_priorities.push({92, {sc2::UNIT_TYPEID::ZERG_ZERGLING, InitialAttack}});
    }
    build_priorities.push({88, {sc2::UNIT_TYPEID::ZERG_OVERLORD, this->Scouts}});
    for (int i = 0; i < 4; ++i) {
        build_priorities.push({86, {sc2::UNIT_TYPEID::ZERG_ROACH, InitialAttack}});
    }
    for (int i = 0; i < 10; ++i) {
        build_priorities.push({85, {sc2::UNIT_TYPEID::ZERG_ZERGLING, InitialAttack}});
    }
    build_priorities.push({84, {sc2::UNIT_TYPEID::ZERG_RAVAGER, InitialAttack}});
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
    this->controller.step();
    for (const auto& base : baseLocations) {
        Debug()->DebugSphereOut(base, 1.5f, sc2::Colors::Green);
    }
    Debug()->SendDebug();
}

bool BasicSc2Bot::BuildUnit(const sc2::UNIT_TYPEID& type) {
    switch (type) {
        case sc2::UNIT_TYPEID::ZERG_ZERGLING:
            return this->BuildZergling();
        case sc2::UNIT_TYPEID::ZERG_ROACH:
            return this->BuildRoach();
        case sc2::UNIT_TYPEID::ZERG_RAVAGER:
            return this->BuildRavager();
        case sc2::UNIT_TYPEID::ZERG_QUEEN:
            return this->BuildQueen();
        case sc2::UNIT_TYPEID::ZERG_DRONE:
            return this->BuildDrone();
        case sc2::UNIT_TYPEID::ZERG_OVERLORD:
            return this->BuildOverlord();
        case sc2::UNIT_TYPEID::ZERG_EXTRACTOR:
            return this->BuildExtractor();
        case sc2::UNIT_TYPEID::ZERG_HATCHERY:
            return this->BuildHatchery();
        case sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL:
            return this->BuildSpawningPool();
        default:
            return false;
    }
};

/**
 * @brief Handles unit creation events.
 *
 * This function is called whenever a new unit is created. It checks the type
 * of the created unit and performs specific actions based on the unit type.
 *
 * @param unit Pointer to the newly created unit.
 */
void BasicSc2Bot::OnUnitCreated(const Unit *unit) {
    if (!assignment_priorities[unit->unit_type.ToType()].empty()) {
        auto top = assignment_priorities[unit->unit_type.ToType()].top();
        if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_DRONE) {
            std::cout << "CREATED: " << (int)top.second->unitTask << "\n";
        }
        (*(top.second)).addUnit(AllyUnit(unit, top.second->unitTask, top.second));
        assignment_priorities[unit->unit_type.ToType()].pop();
    }
}

bool BasicSc2Bot::HasUnitType(sc2::UNIT_TYPEID type) const {
    auto units = Observation()->GetUnits(Unit::Alliance::Self, [type](const Unit& unit) {
        return unit.unit_type == type;
    });
    return !units.empty();
}

bool BasicSc2Bot::HasEnoughSupply(unsigned int requiredSupply) const {
    int current = Observation()->GetFoodUsed();
    int max = Observation()->GetFoodCap();
    return (current + requiredSupply) <= max;
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
    std::vector<std::pair<int, std::pair<sc2::UNIT_TYPEID, UnitGroup*>>> failed_builds;
    // Process the priority queue
    while (!this->build_priorities.empty()) {
        // Get a reference to the top element to avoid unnecessary copying
        const auto& top = this->build_priorities.top();
        // Attempt to build the unit
        if (!this->BuildUnit(top.second.first)) {
            failed_builds.push_back(std::move(top)); // Add failed item to the list
        } else {
            // Record successful build into assignment priorities
            assignment_priorities[top.second.first].push({top.first, top.second.second});
        }

        // Remove the processed element from the queue
        this->build_priorities.pop();
    }

    // Reinsert failed builds into the priority queue
    for (const auto& item : failed_builds) {
        this->build_priorities.push(item);
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
    if (larva.empty()) {
        return false;
    }
    if (!this->HasEnoughSupply(1)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }
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
    const ObservationInterface *observation = Observation();
    Units larva = GetIdleLarva();
    if (!larva.empty() && observation->GetMinerals() >= 100) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_OVERLORD);
        return true;
    }
    return false;
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
    Units larva = GetIdleLarva();
    Units spawning_pool = GetConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);

    if (!this->HasEnoughSupply(1)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }

    if (!larva.empty() && observation->GetMinerals() >= 25 && !spawning_pool.empty()) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_ZERGLING);
        return true;
    }
    return false;
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
    Units hatchery = GetConstructedBuildings(sc2::UNIT_TYPEID::ZERG_HATCHERY);
    
    Units spawning_pool = GetConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
    Units larva = GetIdleLarva();

    if (larva.empty()) {
        return false;
    }
    if (!this->HasEnoughSupply(2)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }

    if (!hatchery.empty() && !spawning_pool.empty() && observation->GetMinerals() >= 150) {
        Actions()->UnitCommand(hatchery[0], ABILITY_ID::TRAIN_QUEEN);
        return true;
    }
    return false;
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
    Units larva = GetIdleLarva();
    if (!HasUnitType(sc2::UNIT_TYPEID::ZERG_ROACHWARREN)) {

        return false;
    }
    if (!this->HasEnoughSupply(1)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }
    if (!larva.empty() && observation->GetMinerals() >= 75 && observation->GetVespene() >= 25) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_ROACH);
        return true;
    }
    return false;
}

#define CLUSTER_DISTANCE 20.0f

void BasicSc2Bot::initializeBaseLocations() {
    sc2::Point3D starting_base = Observation()->GetStartLocation();
    baseLocations.push_back(starting_base);

    const auto& units = Observation()->GetUnits(sc2::Unit::Alliance::Neutral, [](const Unit &unit) {
        return unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD ||
            unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD750 ||
            unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_VESPENEGEYSER;
    });

    std::map<unsigned int, unsigned int> clusterSize;

    for (const auto* unit : units) {
        bool added_to_cluster = false;

        // Compare this resource to existing clusters
        for (unsigned int i = 0; i < baseLocations.size(); ++i) {
            if (DistanceSquared2D(unit->pos, baseLocations[i]) < CLUSTER_DISTANCE * CLUSTER_DISTANCE) { // Within range
                baseLocations[i] = (baseLocations[i] * clusterSize[i] + unit->pos)/(++clusterSize[i]);
                added_to_cluster = true;
                break;
            }
        }

        if (!added_to_cluster) {
            baseLocations.push_back(unit->pos);
        }
    }

    std::sort(baseLocations.begin(), baseLocations.end(), [&starting_base](const sc2::Point3D& a, const sc2::Point3D& b) {
        return DistanceSquared2D(a, starting_base) < DistanceSquared2D(b, starting_base);
    });

    std::cout << "Identified base locations:\n";
    for (const auto& location : baseLocations) {
        std::cout << "Base at: (" << location.x << ", " << location.y << ", " << location.z << ")\n";
    }
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
    const ObservationInterface *observation = Observation();

    std::vector<const Unit*> roaches = GetIntermediates(sc2::UNIT_TYPEID::ZERG_ROACH);
    if (roaches.empty()) {
        if (this->BuildRoach()) {
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_ROACH].push({999999,this->Intermediates});
        }
        return false;
    }
    if (!this->HasEnoughSupply(1)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }
    if (observation->GetMinerals() >= 25 && observation->GetVespene() >= 75) {
        Actions()->UnitCommand(roaches[0], ABILITY_ID::MORPH_RAVAGER);
        return true;
    }
    return false;
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
    std::vector<const Unit*> drones = GetIntermediates(UNIT_TYPEID::ZERG_DRONE);
    if (drones.empty()) {
        if ((assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].empty() ||
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].top().second != this->Intermediates) && BuildDrone()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].push({999999,this->Intermediates});
        }
        return false;
    }
    if (observation->GetMinerals() >= 200) {
        Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_SPAWNINGPOOL);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_SPAWNINGPOOL, buildLocation);
            return true;
        }
    }
    return false;
}

std::vector<const sc2::Unit*> BasicSc2Bot::GetBuildingsNearLocation(sc2::Point2D loc, const sc2::UNIT_TYPEID& type, float radius, bool isBuilt = true) {
    std::vector<const sc2::Unit*> result;

    if (!this->Buildings) {
        std::cerr << "Error: Buildings pointer is null.\n";
        return result;
    }

    for (auto building : this->Buildings->units) {
        if (building.unit != nullptr && building.unit->is_alive && building.unit->health > 0 && 
            building.unit->unit_type == type && (!isBuilt || building.unit->build_progress == 1.0f) &&
            Distance2D(building.unit->pos, loc) < radius*radius) {
                result.push_back(building.unit);
        }
    }
    return result;
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
    std::vector<const Unit*> drones = GetIntermediates(UNIT_TYPEID::ZERG_DRONE);

    if (drones.empty()) {
        if ((assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].empty() ||
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].top().second != Intermediates) && BuildDrone()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].push({999999,this->Intermediates});
        }
        return false;
    }
    if (observation->GetMinerals() >= 25) {
        const auto& geysers = Observation()->GetUnits(sc2::Unit::Alliance::Neutral, [](const Unit &unit) {
            return unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_VESPENEGEYSER ||
                unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_PROTOSSVESPENEGEYSER ||
                unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_SPACEPLATFORMGEYSER;
        });
        for (const auto& loc: baseLocations) {
            for (const auto &geyser : geysers) {
                float distance = Distance2D(geyser->pos, loc);
                if (distance < BASE_SIZE) {
                    Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_EXTRACTOR, geyser);
                    return true;
                }
            }
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
    Units workers = GetIdleWorkers();
    int assignedWorkers = 0;
    for (const auto &worker : workers) {
        if (assignedWorkers >= 3)
            break;
        Actions()->UnitCommand(worker, ABILITY_ID::SMART, extractor);
        assignedWorkers++;
    }
}

std::vector<const sc2::Unit*> BasicSc2Bot::GetIntermediates(const sc2::UNIT_TYPEID& type) {
    std::vector<const sc2::Unit*> result;

    if (!this->Intermediates) {
        std::cerr << "Error: Intermediates pointer is null.\n";
        return result;
    }

    for (const auto& building : this->Intermediates->units) {
        if (building.unit != nullptr && building.unit->is_alive && building.unit->health > 0 && 
            building.unit->unit_type == type) {
                bool is_building = false;
                for (const auto& order : building.unit->orders) {
                    if (order.ability_id == sc2::ABILITY_ID::BUILD_HATCHERY ||
                        order.ability_id == sc2::ABILITY_ID::BUILD_EXTRACTOR ||
                        order.ability_id == sc2::ABILITY_ID::BUILD_SPAWNINGPOOL ||
                        order.ability_id == sc2::ABILITY_ID::BUILD_ROACHWARREN) {
                        is_building = true;
                        break;
                    }
                }
                if (!is_building) {
                    result.push_back(building.unit);
                }
        }
    }
    return result;
}

std::vector<const sc2::Unit*> BasicSc2Bot::GetBuildings(const sc2::UNIT_TYPEID& type, bool isBuilt = true) {
    std::vector<const sc2::Unit*> result;

    if (!this->Buildings) {
        std::cerr << "Error: Buildings pointer is null.\n";
        return result;
    }

    for (auto building : this->Buildings->units) {
        if (building.unit != nullptr && building.unit->is_alive && building.unit->health > 0 && 
            building.unit->unit_type == type && (!isBuilt || building.unit->build_progress == 1.0f)) {
                result.push_back(building.unit);
        }
    }
    return result;
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
    std::vector<const Unit*> drones = GetIntermediates(UNIT_TYPEID::ZERG_DRONE);
    if (drones.empty()) {
        if ((assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].empty() ||
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].top().second != Intermediates) && BuildDrone()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].push({999999,this->Intermediates});
        }
        return false;
    }
    if (observation->GetMinerals() >= 300) {
        Point2D buildLocation = FindExpansionLocation();
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_HATCHERY, buildLocation);
            return true;
        }
    }

    return false;
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
    std::vector<const Unit*> drones = GetIntermediates(UNIT_TYPEID::ZERG_DRONE);
    if (drones.empty()) {
        if ((assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].empty() ||
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].top().second != this->Intermediates) && BuildDrone()) {
        return false;
    }
    if (!drones.empty() && observation->GetMinerals() >= 150) {
        Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_ROACHWARREN);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_ROACHWARREN, buildLocation);
            return true;
        }
    }

    return false;
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
    Units spawning_pool =
        GetConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
    ;
    if (!spawning_pool.empty() && observation->GetMinerals() >= 100 &&
        observation->GetVespene() >= 100) {
        Actions()->UnitCommand(spawning_pool[0],
                               ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST);
        built = true;
    }
    return built;
}

#define HATCHERY_RADIUS 15.0f
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
    for (const auto& loc: baseLocations) {
        std::vector<const sc2::Unit*> hatcheries = GetBuildingsNearLocation(loc, UNIT_TYPEID::ZERG_HATCHERY, HATCHERY_RADIUS, false);
        if (hatcheries.empty()) {
            for (float dx = 0; abs(dx) <= HATCHERY_RADIUS; dx = -(abs(dx) + 1.0f)) {
                for (float dy = 0; abs(dy) <= HATCHERY_RADIUS; dx = -(abs(dy) + 1.0f)) {
                    Point2D buildLocation = Point2D(loc.x + dx, loc.y + dy);
                    if (Query()->Placement(ABILITY_ID::BUILD_HATCHERY, buildLocation)) {
                        return buildLocation;
                    }
                }
            }
        }
    }
    return Point2D(0,0);
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
        return unit.unit_type == UNIT_TYPEID::ZERG_DRONE &&
               (unit.orders.empty() ||
                unit.orders[0].ability_id == ABILITY_ID::HARVEST_GATHER ||
                unit.orders[0].ability_id == ABILITY_ID::HARVEST_RETURN);
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
 * @brief Retrieves a list of fully constructed buildings of a specific type.
 *
 * @param type The UNIT_TYPEID of the building to search for.
 * @return Units A collection of Unit objects representing the constructed
 * buildings.
 */
Units BasicSc2Bot::GetConstructedBuildings(UNIT_TYPEID type) {
    return Observation()->GetUnits(
        Unit::Alliance::Self, [type](const Unit &unit) {
            return unit.build_progress == 1.0f && unit.unit_type == type;
        });
}
