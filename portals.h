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
  optional<short> getDistanceToNearest(Vec2) const;

  SERIALIZATION_DECL(Portals)

  private:
  void recalculateDistances(Level*);
  vector<optional<Vec2>> SERIAL(matchings);
  Table<optional<short>> SERIAL(distanceToNearest);
};
