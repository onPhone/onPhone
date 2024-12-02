#ifndef UNITGROUP_H
#define UNITGROUP_H

#include "sc2-includes.h"
#include "enums_constants.h"

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

#endif // UNITGROUP_H