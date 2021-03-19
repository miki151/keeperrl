#pragma once

#include "util.h"
#include "position.h"
#include "position_map.h"

RICH_ENUM(ZoneId,
  FETCH_ITEMS,
  PERMANENT_FETCH_ITEMS,
  STORAGE_EQUIPMENT,
  STORAGE_RESOURCES,
  QUARTERS1,
  QUARTERS2,
  QUARTERS3,
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
  void onDestroyOrder(Position);
  const PositionSet& getPositions(ZoneId) const;
  void setHighlights(Position, ViewIndex&) const;
  bool canSet(Position, ZoneId, const Collective*) const;
  void tick();

  SERIALIZATION_DECL(Zones)

  private:
  EnumMap<ZoneId, PositionSet> SERIAL(positions);
  PositionMap<EnumSet<ZoneId>> SERIAL(zones);
};
