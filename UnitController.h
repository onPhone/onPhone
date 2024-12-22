#pragma once

class OnPhone;
struct AllyUnit;

struct UnitController {
    OnPhone &bot;
    UnitController(OnPhone &bot);
    virtual void step(AllyUnit &unit) = 0;
    virtual void onDeath(AllyUnit &unit) = 0;
    virtual void underAttack(AllyUnit &unit);
    void base_step(AllyUnit &unit);
};