#pragma once

#include "UnitController.h"
#include "sc2-includes.h"

struct WorkerController : public UnitController {
    WorkerController(OnPhone &bot);
    void step(AllyUnit &unit);
    void underAttack(AllyUnit &unit);
    void onDeath(AllyUnit &unit);
    void extract(AllyUnit &unit);
    void mine(AllyUnit &unit);
    void getMostDangerous();
    const sc2::Unit *most_dangerous_all = nullptr;
    const sc2::Unit *most_dangerous_ground = nullptr;
};