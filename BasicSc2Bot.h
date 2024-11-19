#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2api/sc2_unit_filters.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_arg_parser.h"
#include "sc2utils/sc2_manage_process.h"
#include <queue>

class BasicSc2Bot : public sc2::Agent {
  public:
    virtual void OnGameStart() override;
    virtual void OnStep() override;
    virtual void OnUnitCreated(const sc2::Unit *unit) override;
    virtual void OnUnitDestroyed(const sc2::Unit *unit) override;
    virtual void OnBuildingConstructionComplete(const sc2::Unit *unit) override;

  private:
    sc2::Units constructedBuildings[4]{};
    std::deque<std::pair<int, std::function<bool()>>> buildOrder;
    sc2::Point2D enemyLoc;
    sc2::Point2D rallyPoint;
    bool isAttacking = false;

    void ExecuteBuildOrder();
    void StartAttack(const sc2::Point2D &loc);
    bool AttackMostDangerous();
    bool BuildDrone();
    bool BuildOverlord();
    bool BuildZergling();
    bool BuildSpawningPool();
    bool BuildExtractor();
    bool BuildHatchery();
    bool BuildQueen();
    bool BuildRoachWarren();
    bool ResearchMetabolicBoost();
    bool BuildRoach();
    bool BuildRavager();
    sc2::Units GetIdleWorkers();
    sc2::Units GetIdleLarva();
    void AssignWorkersToExtractor(const sc2::Unit *extractor);
    int GetBuildingIndex(sc2::UNIT_TYPEID type);
    sc2::Point2D FindExpansionLocation();
    sc2::Point2D FindHatcheryPlacement(const sc2::Unit *mineral_field);
    sc2::Point2D FindPlacementForBuilding(sc2::ABILITY_ID ability_type);
};

#endif
