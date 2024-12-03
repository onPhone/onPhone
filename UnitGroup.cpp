#include "BasicSc2Bot.h"

UnitGroup::UnitGroup(ROLE unitRole, TASK unitTask, int sizeTrigger)
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