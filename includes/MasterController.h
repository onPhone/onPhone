#pragma once

#include "AttackController.h"
#include "ScoutController.h"
#include "WorkerController.h"
#include "sc2-includes.h"

class OnPhone;
struct UnitGroup;

struct MasterController {
    OnPhone &bot;
    WorkerController worker_controller;
    ScoutController scout_controller;
    AttackController attack_controller;
    std::vector<UnitGroup> unitGroups;
    MasterController(OnPhone &bot);
    void addUnitGroup(UnitGroup unit);
    void step();
};
