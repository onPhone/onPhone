#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "AllyUnit.h"
#include "MasterController.h"
#include "UnitGroup.h"
#include "sc2-includes.h"

class BasicSc2Bot : public sc2::Agent {
  public:
    BasicSc2Bot();
    virtual void OnGameStart() override;
    virtual void OnStep() override;
    virtual void OnUnitCreated(const sc2::Unit *unit) override;
    virtual void OnUnitDestroyed(const sc2::Unit *unit) override;
    virtual void OnBuildingConstructionComplete(const sc2::Unit *unit) override;

    MasterController controller;
    UnitGroup *Scouts;
    UnitGroup *Larva;
    UnitGroup *Attackers;
    UnitGroup *Workers;
    sc2::Point2D enemyLoc;
    sc2::Point2D startLoc;
    sc2::Point2D mapCenter;

  private:
    sc2::Units constructedBuildings[4]{};
    std::deque<std::pair<int, std::function<bool()>>> buildOrder;

    void AssignWorkersToExtractor(const sc2::Unit *extractor);
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
    sc2::Point2D FindExpansionLocation();
    sc2::Point2D FindHatcheryPlacement(const sc2::Unit *mineral_field);
    sc2::Point2D FindPlacementForBuilding(sc2::ABILITY_ID ability_type);
    void GetEnemyUnitLocations();
    int GetBuildingIndex(sc2::UNIT_TYPEID type);
    sc2::Units GetIdleLarva();
    bool IsGeyser(const sc2::Unit &unit);
    void OnBuildingDestruction(const sc2::Unit *unit);
    bool ResearchMetabolicBoost();
    void tryInjection();
};

#endif
