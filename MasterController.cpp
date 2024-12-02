#include "BasicSc2Bot.h"

MasterController::MasterController(BasicSc2Bot &bot)
    : bot(bot), worker_controller(bot), scout_controller(bot), attack_controller(bot){};

/**
 * @brief Adds a unit group to the master controller.
 *
 * This function adds a unit group to the master controller.
 *
 * @param unitGroup The unitGroup group to add
 */
void MasterController::addUnitGroup(UnitGroup unitGroup) { unitGroups.push_back(unitGroup); }