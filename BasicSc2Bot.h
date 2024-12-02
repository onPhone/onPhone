#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "cpp-sc2/include/sc2api/sc2_common.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2api/sc2_unit_filters.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_arg_parser.h"
#include "sc2utils/sc2_manage_process.h"
#include <sc2api/sc2_typeenums.h>

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
    FAST_SCOUT,
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
    const sc2::Unit *unit;
    TASK unitTask = TASK::UNSET;
    UnitGroup *group = nullptr;
    sc2::UNIT_TYPEID unitType; // added due to role specific tasks being used for on death triggers
    float priorHealth;
    sc2::Point2D priorPos;
    AllyUnit(const sc2::Unit *unit, TASK task, UnitGroup *group);
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
    BasicSc2Bot &bot;
    UnitController(BasicSc2Bot &bot);
    virtual void step(AllyUnit &unit) = 0;
    virtual void onDeath(AllyUnit &unit) = 0;
    virtual void underAttack(AllyUnit &unit);
    void base_step(AllyUnit &unit);
};

struct ScoutController : public UnitController {
    ScoutController(BasicSc2Bot &bot);
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

struct WorkerController : public UnitController {
    WorkerController(BasicSc2Bot &bot);
    void step(AllyUnit &unit);
    void underAttack(AllyUnit &unit);
    void onDeath(AllyUnit &unit);
    void extract(AllyUnit &unit);
    void mine(AllyUnit &unit);
};

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

struct MasterController {
    BasicSc2Bot &bot;
    WorkerController worker_controller;
    ScoutController scout_controller;
    AttackController attack_controller;
    std::vector<UnitGroup> unitGroups;
    MasterController(BasicSc2Bot &bot);
    void addUnitGroup(UnitGroup unit);
    void step();
};

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
