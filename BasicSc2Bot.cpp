#include "BasicSc2Bot.h"
#include "cpp-sc2/include/sc2api/sc2_typeenums.h"
#include <iostream>
#include <limits>
#include <sc2api/sc2_typeenums.h>
#include <sc2api/sc2_unit_filters.h>
#include <cstdlib>

using namespace sc2;

#define EPSILON 0.000001
#define BASE_SIZE 15.0f

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
    bot->Actions()->UnitCommand(unit.unit, sc2::ABILITY_ID::SMART, bot->waypoints[0]);
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

#define ENEMY_EPSILON 1.0f

std::vector<sc2::Point3D> BasicSc2Bot::GetEnemyUnitLocations() {
    const ObservationInterface* observation = Observation();
    std::vector<sc2::Point3D> enemy_unit_locations;

    // Get all enemy units that are either visible or in snapshot
    Units enemy_units = observation->GetUnits(sc2::Unit::Alliance::Enemy, [](const sc2::Unit& unit) {
        return unit.unit_type.ToType() != sc2::UNIT_TYPEID::INVALID &&  // Valid unit
               (unit.display_type == sc2::Unit::DisplayType::Visible || // Visible or
                unit.display_type == sc2::Unit::DisplayType::Snapshot); // Snapshot
    });

    // Add the positions of these units to the list
    for (const auto& enemy_unit : enemy_units) {
        enemy_unit_locations.push_back(enemy_unit->pos);

        // Log unit type and location for debugging
        std::cout << "Enemy " << static_cast<int>(enemy_unit->unit_type.ToType()) 
                  << " at (" << enemy_unit->pos.x 
                  << ", " << enemy_unit->pos.y 
                  << ", " << enemy_unit->pos.z << ").\n";
    }

    return enemy_unit_locations;
}



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

void BasicSc2Bot::addBuild(int priority, const sc2::UNIT_TYPEID& type, UnitGroup*& group) {
    if (group == this->Buildings || type == sc2::UNIT_TYPEID::ZERG_RAVAGER) {
        ++buildOrders[type];
        std::cout<< "ADDED: " << priority << " " << buildOrders[type] << "\n";
    }
    build_priorities.push({priority, {type, group}});
}

void BasicSc2Bot::addBuild(int priority, const sc2::ABILITY_ID& research_ability, const sc2::UNIT_TYPEID& building) {
    research_priorities.push({priority, {research_ability, building}});
}

void BasicSc2Bot::tryInjection() {
    const ObservationInterface* observation = Observation();

    Units queens = observation->GetUnits(Unit::Alliance::Self, [](const sc2::Unit& unit) {
        return unit.unit_type == sc2::UNIT_TYPEID::ZERG_QUEEN && unit.energy >= 25;
    });

    if (queens.empty()) {
        return;
    }

    Units hatcheries = observation->GetUnits(Unit::Alliance::Self, [](const sc2::Unit& unit) {
        return unit.unit_type == sc2::UNIT_TYPEID::ZERG_HATCHERY || 
               unit.unit_type == sc2::UNIT_TYPEID::ZERG_LAIR;
    });

    if (hatcheries.empty()) {
        return;
    }

    for (const auto& hatchery : hatcheries) {
        bool already_injected = false;
        for (const auto& order : hatchery->orders) {
            if (order.ability_id == sc2::ABILITY_ID::EFFECT_INJECTLARVA) {
                already_injected = true;
                break;
            }
        }

        if (!already_injected) {
            // Find the closest Queen to this Hatchery
            const sc2::Unit* closest_queen = nullptr;
            float closest_distance = std::numeric_limits<float>::max();

            for (const auto& queen : queens) {
                float distance = Distance2D(queen->pos, hatchery->pos);
                if (distance < closest_distance) {
                    closest_distance = distance;
                    closest_queen = queen;
                }
            }

            if (closest_queen) {
                Actions()->UnitCommand(closest_queen, sc2::ABILITY_ID::EFFECT_INJECTLARVA, hatchery);
            }
        }
    }
}


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

    auto larva = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
        return unit.unit_type == UNIT_TYPEID::ZERG_LARVA;
    });
    for (const Unit* &unit : larva) {
        InitialMiners->addUnit(AllyUnit(unit, this->Intermediates->unitTask, this->Intermediates));
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
        this->Buildings->addUnit(AllyUnit(unit, this->Buildings->unitTask, this->Buildings));
    }

    for (int i = 0; i < 4; ++i) {
        addBuild(100, sc2::UNIT_TYPEID::ZERG_DRONE, InitialMiners);
    }
    addBuild(97, sc2::UNIT_TYPEID::ZERG_EXTRACTOR, this->Buildings);
    for (int i = 0; i < 5; ++i) {
        addBuild(96, sc2::UNIT_TYPEID::ZERG_DRONE, InitialExtractors);
    }
    addBuild(95, sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL, this->Buildings);
    addBuild(94, sc2::UNIT_TYPEID::ZERG_HATCHERY, this->Buildings);
    addBuild(93, sc2::UNIT_TYPEID::ZERG_QUEEN, InitialAttack);
    for (int i = 0; i < 6; ++i) {
        addBuild(92, sc2::UNIT_TYPEID::ZERG_ZERGLING, InitialAttack);
    }
    addBuild(89, ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST, sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);
    addBuild(88, sc2::UNIT_TYPEID::ZERG_OVERLORD, this->Scouts);
    addBuild(87, sc2::UNIT_TYPEID::ZERG_ROACHWARREN, this->Buildings);
    for (int i = 0; i < 4; ++i) {
        addBuild(86, sc2::UNIT_TYPEID::ZERG_ROACH, InitialAttack);
    }
    for (int i = 0; i < 10; ++i) {
        addBuild(85, sc2::UNIT_TYPEID::ZERG_ZERGLING, InitialAttack);
    }
    addBuild(84, sc2::UNIT_TYPEID::ZERG_RAVAGER, InitialAttack);
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
    GetEnemyUnitLocations();
};

bool BasicSc2Bot::ResearchUpgrade(sc2::ABILITY_ID research_ability, sc2::UNIT_TYPEID required_structure) {
    const ObservationInterface* observation = Observation();

    // Find the required structure
    Units research_structures = Observation()->GetUnits(Unit::Alliance::Self, [required_structure](const sc2::Unit& unit) {
        return unit.unit_type == required_structure &&
               unit.build_progress == 1.0f &&  // Fully built
               unit.orders.empty();           // Not already researching
    }); 

    if (research_structures.empty()) {
        return false; // No valid structure available
    }

    // Locate the specific upgrade data
    const auto& all_upgrades = observation->GetUpgradeData(); // Get all upgrade data
    const sc2::UpgradeData* upgrade_data = nullptr;

    for (const auto& upgrade : all_upgrades) {
        if (upgrade.ability_id == research_ability) {
            upgrade_data = &upgrade;
            break;
        }
    }

    if (!upgrade_data) {
        std::cerr << "Invalid research ability ID: " << static_cast<int>(research_ability) << ".\n";
        return false; // Upgrade not found
    }

    // Check resources
    if (observation->GetMinerals() < upgrade_data->mineral_cost ||
        observation->GetVespene() < upgrade_data->vespene_cost) {
        std::cerr << "Not enough resources to research ability: " << static_cast<int>(research_ability) << ".\n";
        return false; // Insufficient resources
    }

    // Issue the research command
    const sc2::Unit* structure = research_structures[0];
    Actions()->UnitCommand(structure, research_ability);

    std::cout << "Researching ability: " << static_cast<int>(research_ability) 
              << " at structure located at (" << structure->pos.x << ", " << structure->pos.y << ").\n";

    return true;
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
        case sc2::UNIT_TYPEID::ZERG_ROACHWARREN:
            return this->BuildRoachWarren();
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
        std::cout << "SIZE: " << assignment_priorities[unit->unit_type.ToType()].size() << "\n";
        auto top = assignment_priorities[unit->unit_type.ToType()].top();
        if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_DRONE) {
            std::cout << "CREATED: " << (int)top.second->unitTask << "\n";
        }
        (*(top.second)).addUnit(AllyUnit(unit, top.second->unitTask, top.second));
        assignment_priorities[unit->unit_type.ToType()].pop();
    } else if (unit->unit_type.ToType() == sc2::UNIT_TYPEID::ZERG_LARVA) {
        this->Intermediates->addUnit(AllyUnit(unit, this->Intermediates->unitTask, this->Intermediates));
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
    std::vector<std::pair<int, std::pair<sc2::ABILITY_ID, sc2::UNIT_TYPEID>>> failed_researches;

    // Process build and research priorities
    while (!this->build_priorities.empty() || !this->research_priorities.empty()) {
        bool process_build = false;

        // Determine whether to process build or research based on priority
        if (this->research_priorities.empty()) {
            process_build = true; // Only builds available
        } else if (this->build_priorities.empty()) {
            process_build = false; // Only research available
        } else {
            // Compare priorities of the top items
            process_build = (this->build_priorities.top().first >= this->research_priorities.top().first);
        }

        if (process_build) {
            // Handle unit building
            const auto& top = this->build_priorities.top();
            if (!this->BuildUnit(top.second.first)) {
                failed_builds.push_back(top);
            } else {
                // Record successful build into assignment priorities
                assignment_priorities[top.second.first].push({top.first, top.second.second});
            }
            this->build_priorities.pop();
        } else {
            // Handle research
            const auto& research_top = this->research_priorities.top();
            if (!this->ResearchUpgrade(research_top.second.first, research_top.second.second)) {
                failed_researches.push_back(research_top);
            }
            this->research_priorities.pop();
        }
    }

    // Reinsert failed builds into the build priority queue
    for (const auto& item : failed_builds) {
        this->build_priorities.push(item);
    }

    // Reinsert failed research into the research priority queue
    for (const auto& item : failed_researches) {
        this->research_priorities.push(item);
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
    std::vector<const Unit*> larva = GetIntermediates(UNIT_TYPEID::ZERG_LARVA);
    //std::cout << "RAN START\n";
    if (larva.empty()) {
        tryInjection();
        return false;
    }
    //std::cout << larva[0] << "\n";
    if (!this->HasEnoughSupply(1)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty() && assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }
    //std::cout << "RAN MID\n";
    if (observation->GetMinerals() >= 50) {
        const sc2::Unit* unit = larva[0];

        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_DRONE);

        auto& intermediates = this->Intermediates->units;
        intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
            [unit](const AllyUnit& ally_unit) {
                return ally_unit.unit == unit; // Match and remove the drone
            }), intermediates.end());

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
    std::vector<const Unit*> larva = GetIntermediates(UNIT_TYPEID::ZERG_LARVA);
    if (larva.empty()) {
        tryInjection();
        return false;
    }
    if (observation->GetMinerals() >= 100) {
        const sc2::Unit* unit = larva[0];

        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);

        auto& intermediates = this->Intermediates->units;
        intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
            [unit](const AllyUnit& ally_unit) {
                return ally_unit.unit == unit; // Match and remove the drone
            }), intermediates.end());

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
    std::vector<const Unit*> larva = GetIntermediates(UNIT_TYPEID::ZERG_LARVA);
    Units spawning_pool = GetConstructedBuildings(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL);

    if (larva.empty()) {
        tryInjection();
        return false;
    }

    if (!this->HasEnoughSupply(1)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty() && assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }

    if (observation->GetMinerals() >= 25 && !spawning_pool.empty()) {
        const sc2::Unit* unit = larva[0];

        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_ZERGLING);

        auto& intermediates = this->Intermediates->units;
        intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
            [unit](const AllyUnit& ally_unit) {
                return ally_unit.unit == unit; // Match and remove the drone
            }), intermediates.end());

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

    if (!this->HasEnoughSupply(2)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty() && assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }

    if (!hatchery.empty() && !spawning_pool.empty() && observation->GetMinerals() >= 150) {
        const sc2::Unit* unit = hatchery[0];
        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_QUEEN);

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
    auto larva = GetIntermediates(UNIT_TYPEID::ZERG_LARVA);

    if (larva.empty()) {
        tryInjection();
        return false;
    }

    if (!HasUnitType(sc2::UNIT_TYPEID::ZERG_ROACHWARREN)) {
        return false;
    }
    if (!this->HasEnoughSupply(1)) {
        auto units = Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.orders.size() > 0 && unit.orders.front().ability_id == sc2::ABILITY_ID::TRAIN_OVERLORD;
        });
        if (units.empty() && assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }
    if (observation->GetMinerals() >= 75 && observation->GetVespene() >= 25) {
        const sc2::Unit* unit = larva[0];

        Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_ROACH);

        auto& intermediates = this->Intermediates->units;
        intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
            [unit](const AllyUnit& ally_unit) {
                return ally_unit.unit == unit; // Match and remove the drone
            }), intermediates.end());

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
        if (units.empty() && assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].empty()) {
            if (this->BuildOverlord()) {
                assignment_priorities[sc2::UNIT_TYPEID::ZERG_OVERLORD].push({999999,this->Scouts});
            }
        }
        return false;
    }
    if (observation->GetMinerals() >= 25 && observation->GetVespene() >= 75) {

        const sc2::Unit* unit = roaches[0];

        Actions()->UnitCommand(unit, ABILITY_ID::MORPH_RAVAGER);

        auto& intermediates = this->Intermediates->units;
        intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
            [unit](const AllyUnit& ally_unit) {
                return ally_unit.unit == unit; // Match and remove the drone
            }), intermediates.end());

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
        if (buildOrders[sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL] > 0 && BuildDrone()) {
            --buildOrders[sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL];
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].push({999999,this->Intermediates});
        }
        return false;
    }
    std::cout << "DRONE COUNT: " << drones.size() << "\n";
    if (observation->GetMinerals() >= 200) {
        Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_SPAWNINGPOOL);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            std::cout << "ACTUALLY RAN\n";
            const sc2::Unit* drone = drones[0];

            Actions()->UnitCommand(drone, ABILITY_ID::BUILD_SPAWNINGPOOL, buildLocation);

            auto& intermediates = this->Intermediates->units;
            intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
                [drone](const AllyUnit& ally_unit) {
                    return ally_unit.unit == drone; // Match and remove the drone
                }), intermediates.end());

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
        if (buildOrders[sc2::UNIT_TYPEID::ZERG_EXTRACTOR] > 0 && BuildDrone()) {
            --buildOrders[sc2::UNIT_TYPEID::ZERG_EXTRACTOR];
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].push({999999,this->Intermediates});
        }
        return false;
    }
    if (observation->GetMinerals() >= 25) {
        const auto& geysers = Observation()->GetUnits(sc2::Unit::Alliance::Neutral, [](const Unit &unit) {
            return unit.unit_type == sc2::UNIT_TYPEID::NEUTRAL_VESPENEGEYSER;
        });
        for (const auto& loc: baseLocations) {
            for (const auto &geyser : geysers) {
                float distance = Distance2D(geyser->pos, loc);
                if (distance < BASE_SIZE) {
                    const sc2::Unit* drone = drones[0];
                    std::cout << drone << "\n";

                    Actions()->UnitCommand(drone, ABILITY_ID::BUILD_EXTRACTOR, geyser);

                    auto& intermediates = this->Intermediates->units;
                    intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
                        [drone](const AllyUnit& ally_unit) {
                            return ally_unit.unit == drone; // Match and remove the drone
                        }), intermediates.end());

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
            
            bool is_busy = false;

            // Check if the unit is busy (building, gathering, or returning resources)
            for (const auto& order : building.unit->orders) {
                if (order.ability_id != sc2::ABILITY_ID::HARVEST_GATHER &&
                    order.ability_id != sc2::ABILITY_ID::HARVEST_RETURN) {
                    is_busy = true;
                    break;
                }
            }

            // Add unit to result if it's not busy
            if (!is_busy) {
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
        if (buildOrders[sc2::UNIT_TYPEID::ZERG_HATCHERY] > 0 && BuildDrone()) {
            --buildOrders[sc2::UNIT_TYPEID::ZERG_HATCHERY];
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].push({999999,this->Intermediates});
        }
        return false;
    }
    std::cout << "GOT THIS FAR\n";
    if (observation->GetMinerals() >= 300) {
        const sc2::Unit* drone = drones[0];
        std::cout << "CLOSE\n";
        Point2D buildLocation = FindExpansionLocation(drone);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drone, ABILITY_ID::BUILD_HATCHERY, buildLocation);
            std::cout << "DONE\n";
            auto& intermediates = this->Intermediates->units;
            intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
                [drone](const AllyUnit& ally_unit) {
                    return ally_unit.unit == drone; // Match and remove the drone
                }), intermediates.end());

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
        if (buildOrders[sc2::UNIT_TYPEID::ZERG_ROACHWARREN] > 0 && BuildDrone()) {
            --buildOrders[sc2::UNIT_TYPEID::ZERG_ROACHWARREN];
            assignment_priorities[sc2::UNIT_TYPEID::ZERG_DRONE].push({999999,this->Intermediates});
        }
        return false;
    }
    if (observation->GetMinerals() >= 150) {
        std::cout << "TEST\n";
        Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_ROACHWARREN);
        if (buildLocation.x != 0 && buildLocation.y != 0) {
            const sc2::Unit* drone = drones[0];

            Actions()->UnitCommand(drone, ABILITY_ID::BUILD_ROACHWARREN, buildLocation);

            auto& intermediates = this->Intermediates->units;
            intermediates.erase(std::remove_if(intermediates.begin(), intermediates.end(),
                [drone](const AllyUnit& ally_unit) {
                    return ally_unit.unit == drone; // Match and remove the drone
                }), intermediates.end());

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

#define HATCHERY_RADIUS 20.0f
#define BASE_RADIUS 12.0f
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
Point2D BasicSc2Bot::FindExpansionLocation(const sc2::Unit* &unit) {
    for (const auto& loc: baseLocations) {
        std::vector<const sc2::Unit*> hatcheries = GetBuildingsNearLocation(loc, UNIT_TYPEID::ZERG_HATCHERY, HATCHERY_RADIUS, false);
        if (hatcheries.empty()) {
            for (float dx = 0; abs(dx) <= BASE_RADIUS; dx = (dx <= 0) ? (abs(dx) + 1.0f) : -dx) {
                for (float dy = 0; abs(dy) <= BASE_RADIUS; dy = (dy <= 0) ? (abs(dy) + 1.0f) : -dy) {
                    sc2::Point2D starting_base = Observation()->GetStartLocation();
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

    for (float dx = 0; abs(dx) <= BASE_RADIUS; dx = (dx <= 0) ? (abs(dx) + 1.0f) : -dx) {
        for (float dy = 0; abs(dy) <= BASE_RADIUS; dy = (dy <= 0) ? (abs(dy) + 1.0f) : -dy) {
            Point2D buildLocation = Point2D(hatchery_location.x + dx, hatchery_location.y + dy);
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
