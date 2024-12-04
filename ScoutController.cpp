#include "BasicSc2Bot.h"

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
 * This function handles the scout unit being under attack by
 * updating the found enemy location.
 *
 * @param unit The scout unit under attack
 */
void ScoutController::underAttack(AllyUnit &unit) {
    Point2D priorPos = unit.priorPos;
    float minDist = std::numeric_limits<float>::max();
    Point2D closestPoint;
    std::vector<Point2D> locations;
    if(unit.unitTask == TASK::FAST_SCOUT) {
        if(fast_locations.empty()) { initializeFastLocations(); }
        locations = fast_locations;
        for(const auto &location : locations) {
            float dist = DistanceSquared2D(location, priorPos);
            if(dist < minDist) {
                minDist = dist;
                closestPoint = location;
            }
        }
        if(this->foundEnemyLocation.x == 0 && this->foundEnemyLocation.y == 0) {
            this->foundEnemyLocation = closestPoint;
            std::cout << "Enemy base found at (VIA BEING ATTACKED) (" << foundEnemyLocation.x
                      << ", " << foundEnemyLocation.y << ")\n";
        }
    }
    if(all_locations.empty()) { initializeAllLocations(); }
    if(unit.unit != nullptr) {
        bot.Actions()->UnitCommand(unit.unit, ABILITY_ID::SMART, all_locations[0]);
    }
};

/**
 * @brief Handles the scout unit dying.
 *
 * This function handles the scout unit dying
 *
 * @param unit The scout unit that died
 */
void ScoutController::onDeath(AllyUnit &unit) {};

/**
 * @brief Initializes the fast scout locations.
 *
 * This function initializes the fast scout locations by
 * identifying the enemy base location and other possible base locations.
 */
void ScoutController::initializeFastLocations() {
    const auto &gameInfo = bot.Observation()->GetGameInfo();
    std::cout << "Enemy Base possible locations:\n";
    for(const auto &location : gameInfo.enemy_start_locations) {
        fast_locations.push_back(location);
        std::cout << "(" << location.x << ", " << location.y << ")\n";
    }
}

/**
 * @brief Initializes all possible locations for scouting.
 *
 * This function initializes all possible locations for scouting by
 * identifying the reachable waypoints on the map.
 */
void ScoutController::initializeAllLocations() {
    const GameInfo &game_info = bot.Observation()->GetGameInfo();
    all_locations.push_back(bot.startLoc);
    Point2D map_center = (game_info.playable_min + game_info.playable_max) * 0.5f;
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

/**
 * @brief Initializes the base locations for scouting.
 *
 * This function initializes the base locations for scouting by
 * identifying the enemy base location and other possible base locations.
 */
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
}