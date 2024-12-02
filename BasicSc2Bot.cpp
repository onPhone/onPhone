#include "BasicSc2Bot.h"
#include "cpp-sc2/include/sc2api/sc2_interfaces.h"
#include "cpp-sc2/include/sc2api/sc2_typeenums.h"
#include <cstddef>
#include <iostream>
#include <limits>

using namespace sc2;

#define EPSILON 0.000001
#define BASE_SIZE 15.0f
#define ENEMY_EPSILON 1.0f
#define CLUSTER_DISTANCE 20.0f

namespace std {
    template <> struct hash<UnitTypeID> {
        size_t operator()(const UnitTypeID &unit_type) const noexcept {
            return std::hash<int>()(static_cast<int>(unit_type.ToType()));
        }
    };
}

/**
 * @brief Checks if an ally unit is moving.
 *
 * This function checks if the given ally unit is currently moving by comparing
 * its current position to its previous position. If the unit has moved more than
 * a small epsilon distance, it is considered to be moving.
 *
 * @return true if the unit is moving, false otherwise
 */
bool AllyUnit::isMoving() const {
    if(this->unit == nullptr) { return false; }

    if(abs(priorPos.x - unit->pos.x) > EPSILON && abs(priorPos.y - unit->pos.y) > EPSILON) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Constructs an AllyUnit object with the given unit, task, and group.
 *
 * This constructor initializes an AllyUnit object with the given unit, task, and group.
 * It also sets the prior health of the unit to its current health.
 *
 * @param unit Pointer to the unit
 * @param task Task to assign to the unit
 * @param group Pointer to the unit group
 */
AllyUnit::AllyUnit(const Unit *unit, TASK task = TASK::UNSET, UnitGroup *group = nullptr) {
    this->unit = unit;
    this->unitTask = task;
    this->priorHealth = unit->health;
    this->group = group;
};

/**
 * @brief Checks if an ally unit is under attack.
 *
 * This function checks if the given ally unit is currently under attack by comparing
 * its current health to its previous health. If the unit's health has decreased, it is
 * considered to be under attack.
 *
 * @return true if the unit is under attack, false otherwise
 */
bool AllyUnit::underAttack() const {
    if(this->unit == nullptr) { return false; }
    return this->unit->health < priorHealth;
};

UnitController::UnitController(BasicSc2Bot &bot) : bot(bot) {};

/**
 * @brief Handles an ally unit being under attack.
 *
 * This function handles an ally unit being under attack.
 *
 * @param unit The ally unit under attack
 */
void UnitController::underAttack(AllyUnit &unit) {};

/**
 * @brief Executes the base step for an ally unit.
 *
 * This function executes the base step for an ally unit, which is common to all unit roles.
 * It calls the appropriate step function based on whether the unit is currently
 * under attack or not.
 *
 * @param unit The ally unit to step
 */
void UnitController::base_step(AllyUnit &unit) {
    if(unit.underAttack()) {
        underAttack(unit);
    } else {
        step(unit);
    }
};

ScoutController::ScoutController(BasicSc2Bot &bot) : UnitController(bot) {};

/**
 * @brief Steps the scout unit.
 *
 * This function steps the scout unit by moving it to the next location
 * based on its the scouting type in its task.
 *
 * @param unit The scout unit to step
 */
void ScoutController::step(AllyUnit &unit) {
    switch(unit.unitTask) {
    case TASK::SCOUT: scoutBase(unit); break;
    case TASK::SCOUT_ALL: scoutAll(unit); break;
    case TASK::FAST_SCOUT: scoutFast(unit); break;
    default: break;
    }
};

/**
 * @brief Scouts the next base location.
 *
 * This function moves the scout unit to the next base location in the list
 * of base locations.
 *
 * @param unit The scout unit to move
 */
void ScoutController::scoutBase(AllyUnit &unit) {
    if(bot.controller.attack_controller.isAttacking && unit.unit != nullptr
       && unit.unit->orders.empty()) {
        if(base_locations.empty()) { initializeBaseLocations(); }
        unit.group->index = (unit.group->index + 1) % base_locations.size();
        bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART, base_locations[unit.group->index]);
    }
};

/**
 * @brief Scouts all base locations.
 *
 * This function moves the scout unit to all base locations in the list
 * of base locations.
 *
 * @param unit The scout unit to move
 */
void ScoutController::scoutAll(AllyUnit &unit) {
    if(all_locations.empty()) { initializeAllLocations(); }
    if(bot.controller.attack_controller.isAttacking && unit.unit != nullptr
       && unit.unit->orders.empty()) {
        unit.group->index = (unit.group->index + 1) % all_locations.size();
        bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART, all_locations[unit.group->index]);
    }
};

/**
 * @brief Fast scouts the enemy base.
 *
 * This function fast scouts the enemy base by moving the scout unit to the
 * next possible enemy base location.
 *
 * @param unit The scout unit to move
 */
void ScoutController::scoutFast(AllyUnit &unit) {
    if(fast_locations.empty()) { initializeFastLocations(); }
    // Scout based on possible enemy base start locations from the game info
    if(unit.unit != nullptr && unit.unit->orders.empty()) {
        if(foundEnemyLocation.x == 0 && foundEnemyLocation.y == 0) {
            unit.group->index = (unit.group->index + 1) % fast_locations.size();
            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART,
                                       fast_locations[unit.group->index]);
        } else {
            unit.unitTask = TASK::SCOUT;
        }
    }
};

/**
 * @brief Handles the scout unit being under attack.
 *
 * This function handles the scout unit being under attack
 *
 * @param unit The scout unit under attack
 */
void ScoutController::underAttack(AllyUnit &unit) {
    onDeath(unit);
    if(all_locations.empty()) { initializeAllLocations(); }
    bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART, all_locations[0]);
};

/**
 * @brief Handles the scout unit dying.
 *
 * This function handles the scout unit dying by
 * and updating the found enemy location if the scout unit dies.
 *
 * @param unit The scout unit that died
 */
void ScoutController::onDeath(AllyUnit &unit) {
    std::cout << "Scout died at (" << unit.priorPos.x << ", " << unit.priorPos.y << ")\n";
    Point2D deathPos = unit.priorPos;
    float minDist = std::numeric_limits<float>::max();
    Point2D closestPoint;
    std::vector<Point2D> locations;
    if(unit.unitTask == TASK::FAST_SCOUT) {
        if(fast_locations.empty()) { initializeFastLocations(); }
        locations = fast_locations;
    } else if(unit.unitTask == TASK::SCOUT) {
        if(base_locations.empty()) { initializeBaseLocations(); }
        locations = base_locations;
    } else {
        if(all_locations.empty()) { initializeAllLocations(); }
        locations = all_locations;
    }
    for(const auto &location : locations) {
        float dist = DistanceSquared2D(location, deathPos);
        if(dist < minDist) {
            minDist = dist;
            closestPoint = location;
        }
    }
    if(this->foundEnemyLocation.x == 0 && this->foundEnemyLocation.y == 0) {
        this->foundEnemyLocation = closestPoint;
        std::cout << "Enemy base found at (VIA DEATH) (" << foundEnemyLocation.x << ", "
                  << foundEnemyLocation.y << ")\n";
    }
};

void ScoutController::initializeFastLocations() {
    const auto &gameInfo = bot.Observation()->GetGameInfo();
    std::cout << "Enemy Base possible locations:\n";
    for(const auto &location : gameInfo.enemy_start_locations) {
        fast_locations.push_back(location);
        std::cout << "(" << location.x << ", " << location.y << ")\n";
    }
}

void ScoutController::initializeAllLocations() {
    const GameInfo &game_info = bot.Observation()->GetGameInfo();
    all_locations.push_back(bot.startLoc);
    Point2D map_center = (game_info.playable_min + game_info.playable_max) * 0.5f;
    std::cout << game_info.playable_min.x << " " << game_info.playable_min.y << "\n";
    std::cout << game_info.playable_max.x << " " << game_info.playable_max.y << "\n";
    const float step_size = BASE_SIZE; // Distance between waypoints
    for(float x = game_info.playable_min.x; x < game_info.playable_max.x; x += step_size) {
        for(float y = game_info.playable_min.y; y < game_info.playable_max.y; y += step_size) {
            Point2D waypoint(x, y);
            // Check if the waypoint is reachable
            if(bot.Query()->PathingDistance(map_center, waypoint) > 0) {
                all_locations.push_back(waypoint);
            }
        }
    }
};

void ScoutController::initializeBaseLocations() {
    Point2D enemyLocation = bot.enemyLoc;
    base_locations.push_back(enemyLocation);

    const auto &units = bot.Observation()->GetUnits(Unit::Alliance::Neutral, [](const Unit &unit) {
        return unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD
               || unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750
               || unit.unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER;
    });

    std::map<unsigned int, unsigned int> clusterSize;

    for(const auto *unit : units) {
        bool added_to_cluster = false;

        // Compare this resource to existing clusters
        for(unsigned int i = 0; i < base_locations.size(); ++i) {
            if(DistanceSquared2D(unit->pos, base_locations[i])
               < CLUSTER_DISTANCE * CLUSTER_DISTANCE) { // Within range
                base_locations[i]
                  = (base_locations[i] * clusterSize[i] + unit->pos) / (++clusterSize[i]);
                added_to_cluster = true;
                break;
            }
        }

        if(!added_to_cluster) { base_locations.push_back(unit->pos); }
    }

    std::sort(base_locations.begin(), base_locations.end(),
              [&enemyLocation](const Point2D &a, const Point2D &b) {
                  return DistanceSquared2D(a, enemyLocation) < DistanceSquared2D(b, enemyLocation);
              });

    std::cout << "Identified base locations:\n";
    for(const auto &location : base_locations) {
        std::cout << "Base at: (" << location.x << ", " << location.y << ")\n";
    }
}

WorkerController::WorkerController(BasicSc2Bot &bot) : UnitController(bot) {};

/**
 * @brief Steps the worker unit.
 *
 * This function steps the worker unit by executing the appropriate action
 * based on the unit's current task.
 *
 * @param unit The worker unit to step
 */
void WorkerController::step(AllyUnit &unit) {
    switch(unit.unitTask) {
    case TASK::EXTRACT: extract(unit); break;
    case TASK::MINE: mine(unit); break;
    default: break;
    }
};

/**
 * @brief Extracts resources from the nearest extractor.
 *
 * This function extracts resources from the nearest extractor by issuing
 * a smart command to the worker unit.
 *
 * @param unit The worker unit to extract resources
 */
void WorkerController::extract(AllyUnit &unit) {
    bool is_extracting = false;
    for(const auto &order : unit.unit->orders) {
        if(order.ability_id == ABILITY_ID::HARVEST_GATHER) {
            const Unit *target = bot.Observation()->GetUnit(order.target_unit_tag);
            if(target != nullptr && target->unit_type == UNIT_TYPEID::ZERG_EXTRACTOR) {
                is_extracting = true;
            }
            break;
        } else if(order.ability_id == ABILITY_ID::HARVEST_RETURN) {
            is_extracting = true;
            break;
        }
    }
    if(!is_extracting) {
        auto extractors = bot.Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
            return unit.unit_type == UNIT_TYPEID::ZERG_EXTRACTOR
                   && unit.assigned_harvesters < unit.ideal_harvesters;
        });
        if(!extractors.empty()) {
            Point3D starting_base = bot.Observation()->GetStartLocation();
            std::sort(extractors.begin(), extractors.end(),
                      [&starting_base](const Unit *a, const Unit *b) {
                          return DistanceSquared2D(a->pos, starting_base)
                                 < DistanceSquared2D(b->pos, starting_base);
                      });
            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART, extractors[0]);
        }
    }
};

/**
 * @brief Mines resources from the nearest mineral field.
 *
 * This function mines resources from the nearest mineral field by issuing
 * a smart command to the worker unit.
 *
 * @param unit The worker unit to mine resources
 */
void WorkerController::mine(AllyUnit &unit) {
    bool is_extracting = false;
    for(const auto &order : unit.unit->orders) {
        if(order.ability_id == ABILITY_ID::HARVEST_GATHER) {
            const Unit *target = bot.Observation()->GetUnit(order.target_unit_tag);
            if(target != nullptr
               && (target->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD
                   || target->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750
                   || target->unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD
                   || target->unit_type == UNIT_TYPEID::NEUTRAL_RICHMINERALFIELD750)) {
                is_extracting = true;
            }
            break;
        } else if(order.ability_id == ABILITY_ID::HARVEST_RETURN) {
            is_extracting = true;
            break;
        }
    }
    if(!is_extracting) {
        auto minerals = bot.Observation()->GetUnits(Unit::Alliance::Neutral, [](const Unit &unit) {
            return (unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD
                    || unit.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750)
                   && unit.mineral_contents != 0;
        });

        if(!minerals.empty()) {
            Point3D starting_base = bot.Observation()->GetStartLocation();

            std::sort(minerals.begin(), minerals.end(),
                      [&starting_base](const Unit *a, const Unit *b) {
                          return DistanceSquared2D(a->pos, starting_base)
                                 < DistanceSquared2D(b->pos, starting_base);
                      });

            for(const auto *mineral : minerals) {
                const auto &workers
                  = bot.Observation()->GetUnits(Unit::Alliance::Self, [](const Unit &unit) {
                        return unit.unit_type == UNIT_TYPEID::ZERG_DRONE;
                    });

                for(const auto *worker : workers) {
                    for(const auto &order : worker->orders) {
                        if(order.ability_id == ABILITY_ID::HARVEST_GATHER
                           && order.target_unit_tag == mineral->tag) {
                            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART, mineral);
                            return;
                        }
                    }
                }
            }
        }
    }
};

/**
 * @brief Handles the worker unit being under attack.
 *
 * This function handles the worker unit being under attack
 *
 * @param unit The worker unit under attack
 */
void WorkerController::underAttack(AllyUnit &unit) {};

/**
 * @brief Handles the worker unit dying.
 *
 * This function handles the worker unit dying
 *
 * @param unit The worker unit that died
 */
void WorkerController::onDeath(AllyUnit &unit) {};

AttackController::AttackController(BasicSc2Bot &bot) : UnitController(bot) {};

/**
 * @brief Steps the given ally attack unit.
 *
 * This function steps the attack unit by executing the appropriate action
 * based on the unit's current task.
 *
 * @param unit The attack unit to step
 */
void AttackController::step(AllyUnit &unit) {
    switch(unit.unitTask) {
    case TASK::ATTACK: attack(unit); break;
    case TASK::RALLY: rally(unit); break;
    default: break;
    }
};

/**
 * @brief Handles the attack unit being under attack.
 *
 * This function handles the attack unit being under attack
 *
 * @param unit The attack unit under attack
 */
void AttackController::underAttack(AllyUnit &unit) {
    // rallyMultiplier -= 0.01f;
    // rallyMultiplier = fmax(0.0f, rallyMultiplier);
    // bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::MOVE_MOVE, bot.mapCenter);
    approachDistance += 1;
};

/**
 * @brief Handles the attack unit dying.
 *
 * This function handles the attack unit dying
 *
 * @param unit The attack unit that died
 */
void AttackController::onDeath(AllyUnit &unit) {};

void AttackController::rally(AllyUnit &unit) {
    if(unit.unit != nullptr) {
        if(bot.enemyLoc.x != 0 && bot.enemyLoc.y != 0) {
            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::MOVE_MOVE, bot.enemyLoc);
            if(DistanceSquared2D(unit.unit->pos, bot.enemyLoc)
               < approachDistance * approachDistance) {
                bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::MOVE_MOVE, bot.mapCenter);
                if(unit.unit->unit_type.ToType() == UNIT_TYPEID::ZERG_RAVAGER) {
                    isAttacking = true;
                }
            }
        } else {
            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::MOVE_MOVE, bot.mapCenter);
        }
    }
};

void AttackController::attack(AllyUnit &unit) {
    if(unit.unit != nullptr) {
        if(canAttackAir(unit) && most_dangerous_all != nullptr) {
            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::ATTACK_ATTACK,
                                       most_dangerous_all->pos);
            if(unit.unit->unit_type.ToType() == UNIT_TYPEID::ZERG_RAVAGER) {
                bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::EFFECT_CORROSIVEBILE,
                                           most_dangerous_all->pos);
            }
        } else if(!canAttackAir(unit) && most_dangerous_ground != nullptr) {
            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::ATTACK_ATTACK,
                                       most_dangerous_ground->pos);
        } else {
            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::ATTACK_ATTACK, bot.enemyLoc);
        }
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
void AttackController::getMostDangerous() {
    most_dangerous_all = nullptr;
    most_dangerous_ground = nullptr;
    // currently attacks weakest enemy unit
    const auto enemy_units = bot.Observation()->GetUnits(Unit::Alliance::Enemy);
    const UnitTypes unit_data = bot.Observation()->GetUnitTypeData();
    float max_danger_all = std::numeric_limits<float>::lowest();
    float max_danger_ground = std::numeric_limits<float>::lowest();
    for(const auto &unit : enemy_units) {
        UnitTypeData current_unit_data
          = unit_data.at(static_cast<uint32_t>(unit->unit_type.ToType()));
        float unit_health = unit->health + unit->shield;
        float unit_DPS;
        if(current_unit_data.weapons.data() != nullptr) {
            unit_DPS = current_unit_data.weapons.data()->damage_
                       * current_unit_data.weapons.data()->attacks
                       * current_unit_data.weapons.data()->speed;
        } else {
            unit_DPS = 0;
        }
        float unit_danger = unit_DPS / (unit_health); // prevent division by 0
        if(DistanceSquared2D(unit->pos, bot.enemyLoc)
           < DistanceSquared2D(unit->pos, bot.Observation()->GetStartLocation())) {
            if(unit_danger > max_danger_all) {
                max_danger_all = unit_danger;
                most_dangerous_all = unit;
            }
            if(!unit->is_flying && unit_danger > max_danger_ground) {
                max_danger_ground = unit_danger;
                most_dangerous_ground = unit;
            }
        }
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
bool AttackController::canAttackAir(AllyUnit &unit) {
    if(unit.unit != nullptr) {
        switch(unit.unit->unit_type.ToType()) {
        case UNIT_TYPEID::ZERG_QUEEN: return true;
        case UNIT_TYPEID::ZERG_RAVAGER: return true;
        case UNIT_TYPEID::ZERG_ROACH: return true;
        default: return false;
        }
    }
    return false;
}

/**
 * @brief Checks if the given unit is a building.
 *
 * This function checks if the given unit is a building by comparing its
 * unit type to a set of known building types.
 *
 * @param unit The unit to check
 * @return true if the unit is a building, false otherwise
 */
bool IsBuilding(const Unit &unit) {
    // Define a set of all building types for faster lookup.
    static const std::unordered_set<UnitTypeID> building_types = {
      UNIT_TYPEID::TERRAN_COMMANDCENTER,
      UNIT_TYPEID::TERRAN_SUPPLYDEPOT,
      UNIT_TYPEID::TERRAN_REFINERY,
      UNIT_TYPEID::TERRAN_BARRACKS,
      UNIT_TYPEID::TERRAN_ENGINEERINGBAY,
      UNIT_TYPEID::TERRAN_MISSILETURRET,
      UNIT_TYPEID::TERRAN_FACTORY,
      UNIT_TYPEID::TERRAN_STARPORT,
      UNIT_TYPEID::TERRAN_ARMORY,
      UNIT_TYPEID::TERRAN_FUSIONCORE,
      UNIT_TYPEID::PROTOSS_NEXUS,
      UNIT_TYPEID::PROTOSS_PYLON,
      UNIT_TYPEID::PROTOSS_ASSIMILATOR,
      UNIT_TYPEID::PROTOSS_GATEWAY,
      UNIT_TYPEID::PROTOSS_FORGE,
      UNIT_TYPEID::PROTOSS_CYBERNETICSCORE,
      UNIT_TYPEID::PROTOSS_PHOTONCANNON,
      UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY,
      UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE,
      UNIT_TYPEID::PROTOSS_DARKSHRINE,
      UNIT_TYPEID::PROTOSS_FLEETBEACON,
      UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL,
      UNIT_TYPEID::ZERG_HATCHERY,
      UNIT_TYPEID::ZERG_EXTRACTOR,
      UNIT_TYPEID::ZERG_SPAWNINGPOOL,
      UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER,
      UNIT_TYPEID::ZERG_HYDRALISKDEN,
      UNIT_TYPEID::ZERG_SPIRE,
      UNIT_TYPEID::ZERG_ULTRALISKCAVERN,
      UNIT_TYPEID::ZERG_INFESTATIONPIT,
      UNIT_TYPEID::ZERG_NYDUSNETWORK,
      UNIT_TYPEID::ZERG_BANELINGNEST,
      UNIT_TYPEID::ZERG_LURKERDENMP,
      UNIT_TYPEID::ZERG_NYDUSCANAL
      // Add any other buildings
    };

    return building_types.find(unit.unit_type) != building_types.end();
}

/**
 * @brief Gets the locations of enemy base if it is visible and sets enemyLoc.
 *
 * This function retrieves the locations of all enemy units that are either
 * visible or in snapshot and then establishes the enemy base location based
 * on whether they are buildings.
 *
 */
void BasicSc2Bot::GetEnemyUnitLocations() {
    const ObservationInterface *observation = Observation();
    std::vector<Point3D> enemy_unit_locations;

    // Get all enemy units that are either visible or in snapshot
    Units enemy_units = observation->GetUnits(Unit::Alliance::Enemy, [](const Unit &unit) {
        return unit.unit_type.ToType() != UNIT_TYPEID::INVALID &&  // Valid unit
               (unit.display_type == Unit::DisplayType::Visible || // Visible or
                unit.display_type == Unit::DisplayType::Snapshot)
               && (unit.unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER
                   || unit.unit_type == UNIT_TYPEID::PROTOSS_NEXUS
                   || unit.unit_type == UNIT_TYPEID::ZERG_HATCHERY);
    });
    if(!enemy_units.empty()) {
        enemyLoc = enemy_units[0]->pos;
        std::cout << "Enemy at (" << enemyLoc.x << ", " << enemyLoc.y << ")\n";
        this->controller.scout_controller.foundEnemyLocation.x = 0;
        this->controller.scout_controller.foundEnemyLocation.y = 0;
    }
    Point2D scoutControllerEnemyLoc = this->controller.scout_controller.foundEnemyLocation;
    if(scoutControllerEnemyLoc.x != 0 && scoutControllerEnemyLoc.y != 0) {
        enemyLoc = scoutControllerEnemyLoc;
    }
}

MasterController::MasterController(BasicSc2Bot &bot)
    : bot(bot), worker_controller(bot), scout_controller(bot), attack_controller(bot) {};

/**
 * @brief Adds a unit group to the master controller.
 *
 * This function adds a unit group to the master controller.
 *
 * @param unitGroup The unitGroup group to add
 */
void MasterController::addUnitGroup(UnitGroup unitGroup) { unitGroups.push_back(unitGroup); }

UnitGroup::UnitGroup(ROLE unitRole, TASK unitTask = TASK::UNSET, int sizeTrigger = 0)
    : unitRole(unitRole), unitTask(unitTask), sizeTrigger(sizeTrigger) {};

/**
 * @brief Adds a unit to the unit group.
 *
 * This function adds a unit to the unit group.
 *
 * @param unit The unit to add
 */
void UnitGroup::addUnit(AllyUnit unit) {
    this->units.push_back(unit);
    if(this->unitTask != TASK::UNSET) { unit.unitTask = this->unitTask; }
}

/**
 * @brief Steps the master controller
 *
 * This function steps the master controller by iterating through all unit groups
 * and executing the base step for each unit in the group.
 */
void MasterController::step() {
    for(auto &unitGroup : this->unitGroups) {
        switch(unitGroup.unitRole) {
        case ROLE::ATTACK:
            if(attack_controller.isAttacking) {
                attack_controller.getMostDangerous();
                unitGroup.unitTask = TASK::ATTACK;
            }
            break;
        default: break;
        }
        std::vector<AllyUnit> new_units;
        for(auto &unit : unitGroup.units) {
            if(unit.unit != nullptr && unit.unit->is_alive && unit.unit->health > 0) {
                if(unitGroup.unitTask != TASK::UNSET) {
                    unit.unitTask = unitGroup.unitTask; // Done this way so if we want to override
                                                        // group tasks, currently temporary
                }
                switch(unitGroup.unitRole) {
                case ROLE::SCOUT: scout_controller.base_step(unit); break;
                case ROLE::ATTACK: attack_controller.base_step(unit); break;
                case ROLE::WORKER: worker_controller.base_step(unit); break;
                default: break;
                }
                unit.priorHealth = unit.unit->health;
                unit.priorPos = unit.unit->pos;
                new_units.push_back(unit);
            } else {
                unit.unit = nullptr;
                switch(unitGroup.unitRole) {
                case ROLE::SCOUT: scout_controller.onDeath(unit); break;
                case ROLE::ATTACK: attack_controller.onDeath(unit); break;
                case ROLE::WORKER: worker_controller.onDeath(unit); break;
                default: break;
                }
            }
        }
        unitGroup.units = new_units;
    }
};

BasicSc2Bot::BasicSc2Bot() : controller(*this) {};

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

    std::size_t enemyLocationCount = Observation()->GetGameInfo().enemy_start_locations.size();
    if(enemyLocationCount == 1) {
        enemyLoc = Observation()->GetGameInfo().enemy_start_locations[0];
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
                if(distance < closest_distance) {
                    closest_distance = distance;
                    closest_queen = queen;
                }
            }

            if(closest_queen) {
                Actions()->UnitCommand(closest_queen, ABILITY_ID::EFFECT_INJECTLARVA, hatchery);
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
        OnBuildingDestruction(unit);
        buildOrder.push_front({16, std::bind(&BasicSc2Bot::BuildExtractor, this)});
        break;
    case UNIT_TYPEID::ZERG_HATCHERY:
        OnBuildingDestruction(unit);
        buildOrder.push_front({17, std::bind(&BasicSc2Bot::BuildHatchery, this)});
        break;
    case UNIT_TYPEID::ZERG_SPAWNINGPOOL:
        OnBuildingDestruction(unit);
        buildOrder.push_front({16, std::bind(&BasicSc2Bot::BuildSpawningPool, this)});
        break;
    default: break;
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
    if(buildOrder.empty()) return;

    const int currentSupply = Observation()->GetFoodUsed();
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

    Units larva = GetIdleLarva();
    if(larva.empty()) {
        tryInjection();
        return false;
    }

    if(observation->GetMinerals() >= OVERLORD_MINERAL_COST) {
        Actions()->UnitCommand(larva.front(), ABILITY_ID::TRAIN_OVERLORD);
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
    const Point2D startLocation = observation->GetStartLocation();

    for(const auto &geyser : geysers) {
        if(Distance2D(geyser->pos, startLocation) < BASE_SIZE) {
            drone->unitTask = TASK::UNSET;
            Actions()->UnitCommand(drone->unit, ABILITY_ID::BUILD_EXTRACTOR, geyser);
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
    std::cout << "Hatchery built at: " << buildLocation.x << ", " << buildLocation.y << std::endl;
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
        return false;
    }

    const Unit *pool = spawning_pool[0];
    if(!pool->orders.empty()
       && pool->orders[0].ability_id == ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST) {
        return false;
    }

    Actions()->UnitCommand(pool, ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST);
    return true;
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

    Units minerals = observation->GetUnits(Unit::Alliance::Neutral, [](const Unit &u) {
        return u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD
               || u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD750;
    });

    std::sort(minerals.begin(), minerals.end(), [this](const Unit *a, const Unit *b) {
        return Distance2D(a->pos, startLoc) < Distance2D(b->pos, startLoc);
    });

    auto it = std::find_if(minerals.begin(), minerals.end(),
                           [this](const Unit *m) { return Distance2D(m->pos, startLoc) > 10.0f; });

    while(it != minerals.end()) {
        const Unit *mineral = *it;

        bool hatchery_nearby = false;
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
    const Point2D hatchery_location
      = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)][0]->pos;
    static const int dx[] = {1, 0, -1, 0};
    static const int dy[] = {0, 1, 0, -1};
    Point2D current = hatchery_location;

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
