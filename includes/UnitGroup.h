#pragma once

#include "constants.h"
#include "sc2-includes.h"

struct AllyUnit;

struct UnitGroup {
    sc2::Point2D Pos;
    int index = 0;
    ROLE unitRole;
    TASK unitTask;
    int sizeTrigger = 0;
    std::vector<AllyUnit> units;
    UnitGroup(ROLE unitRole, TASK unitTask = TASK::UNSET, int sizeTrigger = 0);
    void addUnit(AllyUnit unit);
};