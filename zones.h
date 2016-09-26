#pragma once

#include "util.h"

RICH_ENUM(ZoneId,
  FETCH_ITEMS,
  STORAGE_EQUIPMENT,
  STORAGE_RESOURCES
);

class Position;
class ViewIndex;

class Zones {
  public:

  bool isZone(Position, ZoneId) const;
  void setZone(Position, ZoneId);
  void eraseZone(Position, ZoneId);
  void eraseZones(Position);
  const set<Position>& getPositions(ZoneId) const;
  void setHighlights(Position, ViewIndex&) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EnumMap<ZoneId, set<Position>> SERIAL(zones);
};
