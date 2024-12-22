#pragma once

#include "UnitController.h"
#include "sc2-includes.h"

struct AttackController : public UnitController {
    AttackController(OnPhone &bot);
    void step(AllyUnit &unit);
    void underAttack(AllyUnit &unit);
    void onDeath(AllyUnit &unit);
    void rally(AllyUnit &unit);
    void attack(AllyUnit &unit);
    void getMostDangerous();
    const sc2::Unit *most_dangerous_all = nullptr;
    const sc2::Unit *most_dangerous_ground = nullptr;
    bool isAttacking = false;
    float approachDistance = 30.0f;
};