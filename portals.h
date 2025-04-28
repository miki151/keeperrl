#pragma once

#include "util.h"
#include "position.h"

class Portals {
  public:
  optional<Position> getOtherPortal(Position) const;
  optional<int> getPortalIndex(Position) const;
  bool registerPortal(Position);
  void removePortal(Position);
  optional<short> getDistanceToNearest(Position) const;
  vector<pair<Position, Position>> getMatchedPortals() const;

  SERIALIZATION_DECL(Portals)

  private:
  void recalculateDistances(Level*);
  vector<optional<Position>> SERIAL(matchings);
  unordered_map<LevelId, Table<optional<short>>> SERIAL(distanceToNearest);
};
