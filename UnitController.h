#ifndef UNITCONTROLLER_H
#define UNITCONTROLLER_H

class BasicSc2Bot;
struct AllyUnit;

struct UnitController {
    BasicSc2Bot &bot;
    UnitController(BasicSc2Bot &bot);
    virtual void step(AllyUnit &unit) = 0;
    virtual void onDeath(AllyUnit &unit) = 0;
    virtual void underAttack(AllyUnit &unit);
    void base_step(AllyUnit &unit);
};

#endif // UNITCONTROLLER_H