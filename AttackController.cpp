#include "BasicSc2Bot.h"

AttackController::AttackController(BasicSc2Bot &bot) : UnitController(bot){};

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
    if(unit.unit != nullptr) {
        approachDistance = fmax(approachDistance + 1, Distance2D(unit.unit->pos, bot.enemyLoc) + 1);
    }
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