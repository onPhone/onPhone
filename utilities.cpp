#include "utilities.h"

namespace std {
    template <> struct hash<sc2::UnitTypeID> {
        size_t operator()(const sc2::UnitTypeID &unit_type) const noexcept {
            return std::hash<int>()(static_cast<int>(unit_type.ToType()));
        }
    };
}

bool IsBuilding(const sc2::Unit &unit) {
    // Define a set of all building types for faster lookup.
    static const std::unordered_set<sc2::UnitTypeID> building_types = {
      sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER,
      sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT,
      sc2::UNIT_TYPEID::TERRAN_REFINERY,
      sc2::UNIT_TYPEID::TERRAN_BARRACKS,
      sc2::UNIT_TYPEID::TERRAN_ENGINEERINGBAY,
      sc2::UNIT_TYPEID::TERRAN_MISSILETURRET,
      sc2::UNIT_TYPEID::TERRAN_FACTORY,
      sc2::UNIT_TYPEID::TERRAN_STARPORT,
      sc2::UNIT_TYPEID::TERRAN_ARMORY,
      sc2::UNIT_TYPEID::TERRAN_FUSIONCORE,
      sc2::UNIT_TYPEID::PROTOSS_NEXUS,
      sc2::UNIT_TYPEID::PROTOSS_PYLON,
      sc2::UNIT_TYPEID::PROTOSS_ASSIMILATOR,
      sc2::UNIT_TYPEID::PROTOSS_GATEWAY,
      sc2::UNIT_TYPEID::PROTOSS_FORGE,
      sc2::UNIT_TYPEID::PROTOSS_CYBERNETICSCORE,
      sc2::UNIT_TYPEID::PROTOSS_PHOTONCANNON,
      sc2::UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY,
      sc2::UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE,
      sc2::UNIT_TYPEID::PROTOSS_DARKSHRINE,
      sc2::UNIT_TYPEID::PROTOSS_FLEETBEACON,
      sc2::UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL,
      sc2::UNIT_TYPEID::ZERG_HATCHERY,
      sc2::UNIT_TYPEID::ZERG_EXTRACTOR,
      sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL,
      sc2::UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER,
      sc2::UNIT_TYPEID::ZERG_HYDRALISKDEN,
      sc2::UNIT_TYPEID::ZERG_SPIRE,
      sc2::UNIT_TYPEID::ZERG_ULTRALISKCAVERN,
      sc2::UNIT_TYPEID::ZERG_INFESTATIONPIT,
      sc2::UNIT_TYPEID::ZERG_NYDUSNETWORK,
      sc2::UNIT_TYPEID::ZERG_BANELINGNEST,
      sc2::UNIT_TYPEID::ZERG_LURKERDENMP,
      sc2::UNIT_TYPEID::ZERG_NYDUSCANAL
      // Add any other buildings
    };

    return building_types.find(unit.unit_type) != building_types.end();
}