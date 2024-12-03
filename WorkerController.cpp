#include "BasicSc2Bot.h"

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