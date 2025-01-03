#pragma once

#include "UnitController.h"
#include "sc2-includes.h"

struct ScoutController : public UnitController {
    ScoutController(OnPhone &bot);
    void step(AllyUnit &unit);
    void scoutBase(AllyUnit &unit);
    void scoutAll(AllyUnit &unit);
    void scoutFast(AllyUnit &unit);
    void underAttack(AllyUnit &unit);
    void onDeath(AllyUnit &unit);
    void initializeFastLocations();
    void initializeBaseLocations();
    void initializeAllLocations();
    std::vector<sc2::Point2D> fast_locations;
    std::vector<sc2::Point2D> base_locations;
    std::vector<sc2::Point2D> all_locations;
    sc2::Point2D foundEnemyLocation;
    std::size_t zerglingCount = 0;
};