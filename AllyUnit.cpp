#include "sc2-includes.h"
#include "AllyUnit.h"

using namespace sc2;

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

