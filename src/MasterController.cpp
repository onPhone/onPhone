#include "OnPhone.h"

MasterController::MasterController(OnPhone &bot)
    : bot(bot), worker_controller(bot), scout_controller(bot), attack_controller(bot) {};

/**
 * @brief Adds a unit group to the master controller.
 *
 * This function adds a unit group to the master controller.
 *
 * @param unitGroup The unitGroup group to add
 */
void MasterController::addUnitGroup(UnitGroup unitGroup) { unitGroups.push_back(unitGroup); }

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
        case ROLE::WORKER: worker_controller.getMostDangerous(); break;
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