#pragma once

#include "util.h"
#include "position.h"

class Portals {
  public:
  Portals(Rectangle bounds);
  optional<Vec2> getOtherPortal(Vec2) const;
  optional<int> getPortalIndex(Vec2) const;
  bool registerPortal(Position);
  void removePortal(Position);
  optional<int> getDistanceToNearest(Vec2) const;

  SERIALIZATION_DECL(Portals)

  private:
  void recalculateDistances(WLevel);
  vector<optional<Vec2>> SERIAL(matchings);
  Table<optional<int>> SERIAL(distanceToNearest);
};
