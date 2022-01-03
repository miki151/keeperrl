#include "stdafx.h"
#include "unknown_locations.h"
#include "level.h"


void UnknownLocations::update(const vector<Position>& positions) {
  allLocations.clear();
  locationsByLevel.clear();
  for (auto pos : positions) {
    locationsByLevel[pos.getLevel()->getUniqueId()].push_back(pos.getCoord());
    allLocations.insert(pos);
  }
}

bool UnknownLocations::contains(Position pos) const {
  return allLocations.count(pos);
}

const vector<Vec2>& UnknownLocations::getOnLevel(const Level* level) const {
  if (auto v = getReferenceMaybe(locationsByLevel, level->getUniqueId()))
    return *v;
  else {
    static vector<Vec2> empty;
    return empty;
  }
}

SERIALIZE_DEF(UnknownLocations, locationsByLevel, allLocations)
