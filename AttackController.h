#ifndef ATTACK_CONTROLLER_H
#define ATTACK_CONTROLLER_H

#include "UnitController.h"
#include "sc2-includes.h"

struct AttackController : public UnitController {
    AttackController(BasicSc2Bot &bot);
    void step(AllyUnit &unit);
    void underAttack(AllyUnit &unit);
    void onDeath(AllyUnit &unit);
    void rally(AllyUnit &unit);
    void attack(AllyUnit &unit);
    void getMostDangerous();
    bool canAttackAir(AllyUnit &unit);
    const sc2::Unit *most_dangerous_all = nullptr;
    const sc2::Unit *most_dangerous_ground = nullptr;
    bool isAttacking = false;
    float approachDistance = 30.0f;
};

#endif // ATTACK_CONTROLLER_H