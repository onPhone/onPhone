#include "BasicSc2Bot.h"
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
    if(base_locations.empty()) { initializeBaseLocations(); }
    if(unit.unit != nullptr && unit.unit->orders.empty()) {
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
    if(unit.unit != nullptr && unit.unit->orders.empty()) {
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
            scoutAll(unit);
        }
    }
    // If unit sees the enemy base, set the foundEnemyLocation
    const auto &enemy_units = bot.Observation()->GetUnits(Unit::Alliance::Enemy);
    for(const auto &unit : enemy_units) {
        if(unit->unit_type == UNIT_TYPEID::TERRAN_COMMANDCENTER
           || unit->unit_type == UNIT_TYPEID::PROTOSS_NEXUS
           || unit->unit_type == UNIT_TYPEID::ZERG_HATCHERY) {
            if(foundEnemyLocation.x != 0 && foundEnemyLocation.y != 0) { break; }
            foundEnemyLocation = unit->pos;
            std::cout << "Enemy base found at (" << foundEnemyLocation.x << ", "
                      << foundEnemyLocation.y << ")\n";
            std::cout << "Enemy base type: " << unit->unit_type << "\n";
            break;
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
    // Disabled for now so the zergling goes deep enough to properly detect the buildings, or dies
    // which also allows us to detect the base location.
    // bot->Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART, bot->waypoints[0]);
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
    for(const auto &location : fast_locations) {
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
    Point2D map_center = (game_info.playable_min + game_info.playable_max) * 0.5f;
    std::cout << game_info.playable_min.x << " " << game_info.playable_min.y << "\n";
    std::cout << game_info.playable_max.x << " " << game_info.playable_max.y << "\n";
    const float step_size = 15.0f; // Distance between waypoints
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
    Point3D starting_base = bot.Observation()->GetStartLocation();
    base_locations.push_back(starting_base);

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
              [&starting_base](const Point3D &a, const Point3D &b) {
                  return DistanceSquared2D(a, starting_base) < DistanceSquared2D(b, starting_base);
              });

    std::cout << "Identified base locations:\n";
    for(const auto &location : base_locations) {
        std::cout << "Base at: (" << location.x << ", " << location.y << ", " << location.z
                  << ")\n";
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
void AttackController::underAttack(AllyUnit &unit) {};

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
        if(this->bot.enemyLoc.x != 0 && this->bot.enemyLoc.y != 0) {
            bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::MOVE_MOVE, rallyPoint);
            if(unit.unit->unit_type.ToType() == UNIT_TYPEID::ZERG_RAVAGER
               && DistanceSquared2D(unit.unit->pos, rallyPoint) < BASE_SIZE) {
                isAttacking = true;
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
               && IsBuilding(unit); // Apparently no other way sadly...
    });
    if(!enemy_units.empty() && enemyLoc.x == 0 && enemyLoc.y == 0) {
        enemyLoc = enemy_units[0]->pos;
        std::cout << "Enemy at (" << enemyLoc.x << ", " << enemyLoc.y << ")\n";
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
            attack_controller.rallyPoint = bot.startLoc + (bot.enemyLoc - bot.startLoc) * 0.75f;
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
 * @brief Checks if the bot has enough supply to build a unit.
 *
 * This function checks if the bot has enough supply to build a unit by comparing
 * the current supply used to the maximum supply cap.
 *
 * @param requiredSupply The required supply to build the unit
 * @return true if the bot has enough supply, false otherwise
 */
bool BasicSc2Bot::HasEnoughSupply(unsigned int requiredSupply) const {
    int current = Observation()->GetFoodUsed();
    int max = Observation()->GetFoodCap();
    return (current + requiredSupply) <= max;
}

/**
 * @brief Researches an upgrade.
 *
 * This function researches an upgrade by issuing a research command to the
 * required structure.
 *
 * @param research_ability The ability ID of the upgrade to research
 * @param required_structure The type ID of the structure required to research the upgrade
 * @return true if the upgrade was successfully researched, false otherwise
 */
bool BasicSc2Bot::ResearchUpgrade(ABILITY_ID research_ability, UNIT_TYPEID required_structure) {
    const ObservationInterface *observation = Observation();

    // Find the required structure
    Units research_structures
      = Observation()->GetUnits(Unit::Alliance::Self, [required_structure](const Unit &unit) {
            return unit.unit_type == required_structure && unit.build_progress == 1.0f
                   &&                   // Fully built
                   unit.orders.empty(); // Not already researching
        });

    if(research_structures.empty()) {
        return false; // No valid structure available
    }

    // Locate the specific upgrade data
    const auto &all_upgrades = observation->GetUpgradeData(); // Get all upgrade data
    const UpgradeData *upgrade_data = nullptr;

    for(const auto &upgrade : all_upgrades) {
        if(upgrade.ability_id == research_ability) {
            upgrade_data = &upgrade;
            break;
        }
    }

    if(!upgrade_data) {
        std::cerr << "Invalid research ability ID: " << static_cast<int>(research_ability) << ".\n";
        return false; // Upgrade not found
    }

    // Check resources
    if(observation->GetMinerals() < upgrade_data->mineral_cost
       || observation->GetVespene() < upgrade_data->vespene_cost) {
        return false;
    }

    // Issue the research command
    const Unit *structure = research_structures[0];
    Actions()->UnitCommand(structure, research_ability);

    std::cout << "Researching ability: " << static_cast<int>(research_ability)
              << " at structure located at (" << structure->pos.x << ", " << structure->pos.y
              << ").\n";

    return true;
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
    const auto &gameInfo = Observation()->GetGameInfo();
    startLoc = Observation()->GetStartLocation();
    std::cout << "Start location: (" << startLoc.x << ", " << startLoc.y << ")\n";
    mapCenter = (gameInfo.playable_min + gameInfo.playable_max) * 0.5f;
    std::cout << "Map center: (" << mapCenter.x << ", " << mapCenter.y << ")\n";

    this->controller.addUnitGroup(UnitGroup(ROLE::INTERMEDIATE));
    this->controller.addUnitGroup(UnitGroup(ROLE::SCOUT, TASK::FAST_SCOUT, 3));
    this->controller.addUnitGroup(UnitGroup(ROLE::ATTACK, TASK::RALLY));

    // Retrieve pointers to the corresponding UnitGroups
    this->Larva = &(this->controller.unitGroups[0]);
    this->Scouts = &(this->controller.unitGroups[1]);
    this->Attackers = &(this->controller.unitGroups[2]);

    constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)].push_back(
      Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY))[0]);
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
    GetEnemyUnitLocations();
    ExecuteBuildOrder();
    this->controller.step();
    for(const auto &base : controller.scout_controller.base_locations) {
        Debug()->DebugSphereOut(base, 1.5f, Colors::Green);
    }
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
    case UNIT_TYPEID::ZERG_LARVA: {
        this->Larva->addUnit(AllyUnit(unit, this->Larva->unitTask, this->Larva));
        break;
    }
    case UNIT_TYPEID::ZERG_OVERLORD: {
        // this->Scouts->addUnit(AllyUnit(unit, this->Scouts->unitTask, this->Scouts));
        break;
    }
    case UNIT_TYPEID::ZERG_ZERGLING:
        if(this->Scouts->units.size() < this->Scouts->sizeTrigger) {
            this->Scouts->addUnit(AllyUnit(unit, this->Scouts->unitTask, this->Scouts));
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
    if(larva.empty()) {
        tryInjection();
        return false;
    }
    if(observation->GetMinerals() >= 50 && observation->GetFoodUsed() < observation->GetFoodCap()) {
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
    if(larva.empty()) {
        tryInjection();
        return false;
    }
    if(observation->GetMinerals() >= 100) {
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
    Units spawning_pool = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
    if(larva.empty()) {
        tryInjection();
        return false;
    }
    if(observation->GetMinerals() >= 25 && observation->GetFoodUsed() < observation->GetFoodCap()
       && !spawning_pool.empty()) {
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
    Units hatchery = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)];
    Units spawning_pool = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
    if(!hatchery.empty() && !spawning_pool.empty() && observation->GetMinerals() >= 150
       && observation->GetFoodUsed() + 1 < observation->GetFoodCap()) {
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
    if(larva.empty()) {
        tryInjection();
        return false;
    }
    Units roach_warren = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_ROACHWARREN)];
    if(observation->GetMinerals() >= 75 && observation->GetVespene() >= 25
       && observation->GetFoodUsed() + 1 < observation->GetFoodCap() && !roach_warren.empty()) {
        Actions()->UnitCommand(larva[0], ABILITY_ID::TRAIN_ROACH);
        return true;
    }
    return false;
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
    Units roaches = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_ROACH));
    Units roach_warren = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_ROACHWARREN)];
    if(!roaches.empty() && observation->GetMinerals() >= 25 && observation->GetVespene() >= 75
       && observation->GetFoodUsed() < observation->GetFoodCap() && !roach_warren.empty()) {
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
    Units drones = GetIdleWorkers();
    if(!drones.empty() && observation->GetMinerals() >= 200) {
        Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_SPAWNINGPOOL);
        if(buildLocation.x != 0 && buildLocation.y != 0) {
            Units spawning_pool
              = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_SPAWNINGPOOL, buildLocation);
            return true;
        }
    }
    return false;
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
    Units drones = GetIdleWorkers();
    if(!drones.empty() && observation->GetMinerals() >= 25) {
        Units geysers = observation->GetUnits(Unit::Alliance::Neutral,
                                              IsUnit(UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        for(const auto &geyser : geysers) {
            float distance = Distance2D(geyser->pos, observation->GetStartLocation());
            if(distance < 15) {
                Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_EXTRACTOR, geyser);
                return true;
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
    for(const auto &worker : workers) {
        if(assignedWorkers >= 3) break;
        Actions()->UnitCommand(worker, ABILITY_ID::SMART, extractor);
        ++assignedWorkers;
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
    Units drones = GetIdleWorkers();
    if(!drones.empty() && observation->GetMinerals() >= 300) {
        Point2D buildLocation = FindExpansionLocation();
        if(buildLocation.x != 0 && buildLocation.y != 0) {
            Actions()->UnitCommand(drones[0], ABILITY_ID::BUILD_HATCHERY, buildLocation);
            std::cout << "Hatchery built at: " << buildLocation.x << ", " << buildLocation.y
                      << std::endl;
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
    Units drones = GetIdleWorkers();
    if(!drones.empty() && observation->GetMinerals() >= 150) {
        Point2D buildLocation = FindPlacementForBuilding(ABILITY_ID::BUILD_ROACHWARREN);
        if(buildLocation.x != 0 && buildLocation.y != 0) {
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
    const ObservationInterface *observation = Observation();
    Units spawning_pool = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_SPAWNINGPOOL)];
    if(!spawning_pool.empty() && observation->GetMinerals() >= 100
       && observation->GetVespene() >= 100) {
        Actions()->UnitCommand(spawning_pool[0], ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST);
        return true;
    }
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
    Point2D hatchery_location
      = constructedBuildings[GetBuildingIndex(UNIT_TYPEID::ZERG_HATCHERY)][0]->pos;

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
