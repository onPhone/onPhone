#pragma once

#include "constants.h"
#include "sc2-includes.h"

struct UnitGroup;

struct AllyUnit {
    const sc2::Unit *unit;
    TASK unitTask = TASK::UNSET;
    UnitGroup *group = nullptr;
    sc2::UNIT_TYPEID unitType; // added due to role specific tasks being used for on death triggers
    float priorHealth;
    sc2::Point2D priorPos;
    AllyUnit(const sc2::Unit *unit, TASK task, UnitGroup *group);
    bool underAttack() const;
    bool isMoving() const;
};