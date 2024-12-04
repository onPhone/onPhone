#ifndef MASTER_CONTROLLER_H
#define MASTER_CONTROLLER_H

#include "AttackController.h"
#include "ScoutController.h"
#include "WorkerController.h"
#include "sc2-includes.h"

class BasicSc2Bot;
struct UnitGroup;

struct MasterController {
    BasicSc2Bot &bot;
    WorkerController worker_controller;
    ScoutController scout_controller;
    AttackController attack_controller;
    std::vector<UnitGroup> unitGroups;
    MasterController(BasicSc2Bot &bot);
    void addUnitGroup(UnitGroup unit);
    void step();
};

#endif // MASTER_CONTROLLER_H
