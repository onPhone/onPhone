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

  private:
    std::queue<std::pair<int, std::function<bool()>>> buildOrder;

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
    sc2::Point2D FindPlacementForBuilding(sc2::ABILITY_ID ability_type);
};

#endif
