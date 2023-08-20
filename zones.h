#pragma once

#include "util.h"
#include "position.h"
#include "position_map.h"
#include "unique_entity.h"
#include "entity_map.h"
#include "sectors.h"

RICH_ENUM(ZoneId,
  FETCH_ITEMS,
  PERMANENT_FETCH_ITEMS,
  STORAGE_EQUIPMENT,
  STORAGE_RESOURCES,
  QUARTERS,
  LEISURE,
  GUARD1,
  GUARD2,
  GUARD3
);

class Position;
class ViewIndex;

extern ViewId getViewId(ZoneId);

class Zones {
  public:
  bool isZone(Position, ZoneId) const;
  bool isAnyZone(Position, EnumSet<ZoneId>) const;
  void setZone(Position, ZoneId);
  void eraseZone(Position, ZoneId);
  const PositionSet& getPositions(ZoneId) const;
  void setHighlights(Position, ViewIndex&) const;
  bool canSet(Position, ZoneId, const Collective*) const;
  void tick();
  const PositionSet& getQuarters(UniqueEntity<Creature>::Id) const;
  optional<double> getQuartersLuxury(UniqueEntity<Creature>::Id) const;
  void assignQuarters(UniqueEntity<Creature>::Id, Position);
  struct QuartersInfo {
    optional<UniqueEntity<Creature>::Id> id;
    const PositionSet& positions;
  };
  optional<QuartersInfo> getQuartersInfo(Position) const;
  optional<UniqueEntity<Creature>::Id> getAssignedToQuarters(Position) const;

  SERIALIZATION_DECL(Zones)

  private:
  const PositionSet& getQuartersPositions(Level* level, Sectors::SectorId id) const;
  unordered_map<Level*, Sectors> SERIAL(quartersSectors);
  BiMap<UniqueEntity<Creature>::Id, pair<Level*, Sectors::SectorId>> SERIAL(quarters);
  EnumMap<ZoneId, PositionSet> SERIAL(positions);
  PositionMap<EnumSet<ZoneId>> SERIAL(zones);
  mutable HashMap<pair<Level*, Sectors::SectorId>, PositionSet> quartersPositionCache;
  Sectors& getOrInitSectors(Level*);
};
