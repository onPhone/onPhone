#include "BasicSc2Bot.h"

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