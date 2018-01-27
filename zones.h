#pragma once

#include "util.h"
#include "position.h"

RICH_ENUM(ZoneId,
  FETCH_ITEMS,
  PERMANENT_FETCH_ITEMS,
  STORAGE_EQUIPMENT,
  STORAGE_RESOURCES,
  QUARTERS1,
  QUARTERS2,
  QUARTERS3
);

class Position;
class ViewIndex;

class Zones {
  public:

  bool isZone(Position, ZoneId) const;
  void setZone(Position, ZoneId);
  void eraseZone(Position, ZoneId);
  void eraseZones(Position);
  const PositionSet& getPositions(ZoneId) const;
  void setHighlights(Position, ViewIndex&) const;
  bool canSet(Position, ZoneId, WConstCollective) const;
  void tick();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EnumMap<ZoneId, PositionSet> SERIAL(zones);
};
