#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2api/sc2_unit_filters.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_arg_parser.h"
#include "sc2utils/sc2_manage_process.h"
#include <queue>
#include <vector>
#include <utility>
#include <map>

enum class ROLE {
  SCOUT,
  ATTACK,
  WORKER,
  BUILDING,
  INTERMEDIATE // Refers to groups that become material for others, so drones that become buildings
};

enum class TASK {
  UNSET, // Specifically for group task setting
  ATTACK,
  MINE,
  EXTRACT,
  SCOUT,
  SCOUT_ALL,
  MOVE,
  RALLY
};

class BasicSc2Bot;
struct AllyUnit;
struct UnitGroup;
struct UnitController;
struct ScoutController;
struct WorkerController;
struct AttackController;
struct MasterController;

struct AllyUnit {
    const sc2::Unit* unit;
    TASK unitTask = TASK::UNSET;
    UnitGroup* group = nullptr;
    sc2::UNIT_TYPEID unitType; // added due to role specific tasks being used for on death triggers
    float priorHealth;
    sc2::Point2D priorPos;
    AllyUnit(const sc2::Unit* unit, TASK task, UnitGroup* group);
    bool underAttack() const;
    bool isMoving() const;
};

struct UnitGroup {
  sc2::Point2D Pos;
  int index = 0;
  ROLE unitRole;
  TASK unitTask;
  int sizeTrigger = 0;
  std::vector<AllyUnit> units;
  UnitGroup(ROLE unitRole, TASK unitTask, int sizeTrigger);
  void addUnit(AllyUnit unit);
};

struct UnitController {
    BasicSc2Bot* bot;
    UnitController(BasicSc2Bot* bot);
    virtual void step(AllyUnit& unit) = 0;
    virtual void onDeath(AllyUnit& unit) = 0;
    virtual void underAttack(AllyUnit& unit);
    void base_step(AllyUnit& unit);
};

struct ScoutController : public UnitController {
    ScoutController(BasicSc2Bot* bot);
    void step(AllyUnit& unit);
    void scout(AllyUnit& unit);
    void scout_all(AllyUnit& unit);
    void underAttack(AllyUnit& unit);
    void onDeath(AllyUnit& unit);
};

struct WorkerController : public UnitController {
    WorkerController(BasicSc2Bot* bot);
    void step(AllyUnit& unit);
    void underAttack(AllyUnit& unit);
    void onDeath(AllyUnit& unit);
    void extract(AllyUnit& unit);
    void mine(AllyUnit& unit);
};

struct AttackController : public UnitController {
    AttackController(BasicSc2Bot* bot);
    void step(AllyUnit& unit);
    void underAttack(AllyUnit& unit);
    void onDeath(AllyUnit& unit);
};

struct MasterController {
    BasicSc2Bot* bot;
    WorkerController worker_controller;
    ScoutController scout_controller;
    AttackController attack_controller;
    std::vector<UnitGroup> unitGroups;
    MasterController(BasicSc2Bot* bot);
    void addUnitGroup(UnitGroup unit);
    void step();
};

struct PriorityCompare {
  template <typename T>
  bool operator()(const std::pair<int,T>& p1,const std::pair<int,T>& p2) {
    return p1.first < p2.first;
  }
};

typedef std::pair<int,std::pair<sc2::UNIT_TYPEID,UnitGroup*>> build_priority_item;

class BasicSc2Bot : public sc2::Agent {
  public:
    BasicSc2Bot();
    virtual void OnGameStart() override;
    virtual void OnStep() override;
    virtual void OnUnitCreated(const sc2::Unit *unit) override;

    MasterController controller;
    UnitGroup* Intermediates;
    UnitGroup* Buildings;
    UnitGroup* Scouts;
    std::priority_queue<build_priority_item,
      std::vector<build_priority_item>,
      PriorityCompare> build_priorities;
    std::map<sc2::UNIT_TYPEID, 
      std::priority_queue<std::pair<int,UnitGroup*>,
      std::vector<std::pair<int,UnitGroup*>>,
      PriorityCompare>> assignment_priorities;
    sc2::Point2D enemyLoc;
    std::vector<sc2::Point3D> baseLocations;
    std::vector<sc2::Point2D> waypoints;
    std::vector<sc2::Point3D> enemyBases;
  private:
    void ExecuteBuildOrder();
    void initializeWaypoints();
    void initializeBaseLocations();
    bool HasUnitType(sc2::UNIT_TYPEID type) const;
    std::vector<const sc2::Unit*> GetIntermediates(const sc2::UNIT_TYPEID& type);
    std::vector<const sc2::Unit*> GetBuildings(const sc2::UNIT_TYPEID& type, bool isBuilt);
    std::vector<const sc2::Unit*> GetBuildingsNearLocation(sc2::Point2D loc, const sc2::UNIT_TYPEID& type, float radius, bool isBuilt);
    bool HasEnoughSupply(unsigned int requiredSupply) const;
    bool BuildUnit(const sc2::UNIT_TYPEID& type);
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
    sc2::Units GetConstructedBuildings(sc2::UNIT_TYPEID type);
    sc2::Point2D FindExpansionLocation();
    sc2::Point2D FindPlacementForBuilding(sc2::ABILITY_ID ability_type);
};

#endif
