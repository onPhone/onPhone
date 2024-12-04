#include "utilities.h"

using namespace sc2;

namespace std {
    template <> struct hash<UnitTypeID> {
        size_t operator()(const UnitTypeID &unit_type) const noexcept {
            return std::hash<int>()(static_cast<int>(unit_type.ToType()));
        }
    };
}

/**
 * Checks if a unit is a building.
 * @param unit The unit to check
 * @return true if the unit is a building, false otherwise
 */
bool IsBuilding(const Unit &unit) {
    static const std::unordered_set<UnitTypeID> building_types
      = {UNIT_TYPEID::TERRAN_COMMANDCENTER,   UNIT_TYPEID::TERRAN_SUPPLYDEPOT,
         UNIT_TYPEID::TERRAN_REFINERY,        UNIT_TYPEID::TERRAN_BARRACKS,
         UNIT_TYPEID::TERRAN_ENGINEERINGBAY,  UNIT_TYPEID::TERRAN_MISSILETURRET,
         UNIT_TYPEID::TERRAN_FACTORY,         UNIT_TYPEID::TERRAN_STARPORT,
         UNIT_TYPEID::TERRAN_ARMORY,          UNIT_TYPEID::TERRAN_FUSIONCORE,
         UNIT_TYPEID::PROTOSS_NEXUS,          UNIT_TYPEID::PROTOSS_PYLON,
         UNIT_TYPEID::PROTOSS_ASSIMILATOR,    UNIT_TYPEID::PROTOSS_GATEWAY,
         UNIT_TYPEID::PROTOSS_FORGE,          UNIT_TYPEID::PROTOSS_CYBERNETICSCORE,
         UNIT_TYPEID::PROTOSS_PHOTONCANNON,   UNIT_TYPEID::PROTOSS_ROBOTICSFACILITY,
         UNIT_TYPEID::PROTOSS_TEMPLARARCHIVE, UNIT_TYPEID::PROTOSS_DARKSHRINE,
         UNIT_TYPEID::PROTOSS_FLEETBEACON,    UNIT_TYPEID::PROTOSS_TWILIGHTCOUNCIL,
         UNIT_TYPEID::ZERG_HATCHERY,          UNIT_TYPEID::ZERG_EXTRACTOR,
         UNIT_TYPEID::ZERG_SPAWNINGPOOL,      UNIT_TYPEID::ZERG_EVOLUTIONCHAMBER,
         UNIT_TYPEID::ZERG_HYDRALISKDEN,      UNIT_TYPEID::ZERG_SPIRE,
         UNIT_TYPEID::ZERG_ULTRALISKCAVERN,   UNIT_TYPEID::ZERG_INFESTATIONPIT,
         UNIT_TYPEID::ZERG_NYDUSNETWORK,      UNIT_TYPEID::ZERG_BANELINGNEST,
         UNIT_TYPEID::ZERG_LURKERDENMP,       UNIT_TYPEID::ZERG_NYDUSCANAL};

    return building_types.find(unit.unit_type) != building_types.end();
}