#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "AllyUnit.h"
#include "MasterController.h"
#include "UnitGroup.h"
#include "sc2-includes.h"
#include "utilities.h"

using namespace sc2;

class BasicSc2Bot : public Agent {
  public:
    BasicSc2Bot();
    virtual void OnGameStart() override;
    virtual void OnGameEnd() final;
    virtual void OnStep() override;
    virtual void OnUnitCreated(const Unit *unit) override;
    virtual void OnUnitDestroyed(const Unit *unit) override;
    virtual void OnBuildingConstructionComplete(const Unit *unit) override;

    MasterController controller;
    UnitGroup *Scouts;
    UnitGroup *Larva;
    UnitGroup *Attackers;
    UnitGroup *Workers;
    Point2D enemyLoc;
    Point2D startLoc;
    Point2D mapCenter;
    bool top;
    bool right;

  private:
    Units constructedBuildings[4]{};
    std::deque<std::pair<int, std::function<bool()>>> buildOrder;

    void AssignWorkersToExtractor(const Unit *extractor);
    bool BuildDrone();
    bool BuildExtractor();
    bool BuildHatchery();
    bool BuildOverlord();
    bool BuildQueen();
    bool BuildRavager();
    bool BuildRoach();
    bool BuildRoachWarren();
    bool BuildSpawningPool();
    bool BuildZergling();
    void ExecuteBuildOrder();
    Point2D FindExpansionLocation();
    Point2D FindHatcheryPlacement(const Unit *mineral_field);
    Point2D FindPlacementForBuilding(ABILITY_ID ability_type);
    void GetEnemyUnitLocations();
    int GetBuildingIndex(UNIT_TYPEID type);
    Units GetIdleLarva();
    bool IsGeyser(const Unit &unit);
    void OnBuildingDestruction(const Unit *unit);
    bool ResearchMetabolicBoost();
    void tryInjection();
};

#endif
