#include "stdafx.h"
#include "portals.h"
#include "shortest_path.h"
#include "level.h"

SERIALIZE_DEF(Portals, matchings, distanceToNearest)
SERIALIZATION_CONSTRUCTOR_IMPL(Portals)

optional<int> Portals::getPortalIndex(Vec2 position) const {
  return matchings.findElement(position).map([](int a) {return a / 2;});
}

void Portals::removePortal(Position position) {
  if (auto index = matchings.findElement(position.getCoord())) {
    matchings[*index] = none;
    recalculateDistances(position.getLevel());
  }
}

optional<short> Portals::getDistanceToNearest(Vec2 pos) const {
  return distanceToNearest[pos];
}

void Portals::recalculateDistances(Level* level) {
  vector<Vec2> portals;
  for (auto& portal : matchings)
    if (portal)
      portals.push_back(*portal);
  if (!level) {
    distanceToNearest = Table<optional<short>>(1, 1);
    return;
  }
  distanceToNearest = Table<optional<short>>(level->getBounds());
  auto entryFun = [&](Vec2 pos) {
    if (Position(pos, level).canEnterEmpty({MovementTrait::WALK}))
      return 1;
    else
      return 10000;
  };
  Dijkstra dijkstra(distanceToNearest.getBounds(), portals, 10000, entryFun);
  for (auto& pos : dijkstra.getAllReachable())
    distanceToNearest[pos.first] = (short) pos.second;
}

Portals::Portals(Rectangle bounds) : distanceToNearest(bounds) {
}

int getOtherIndex(int index) {
  return index - index % 2 + (index + 1) % 2;
}

optional<Vec2> Portals::getOtherPortal(Vec2 position) const {
  if (auto index = matchings.findElement(position)) {
    auto otherIndex = getOtherIndex(*index);
    if (otherIndex < matchings.size())
      return matchings[otherIndex];
  }
  return none;
}

bool Portals::registerPortal(Position pos) {
  if (!matchings.contains(pos.getCoord())) {
    bool foundInactive = false;
    for (int i : All(matchings)) {
      int other = getOtherIndex(i);
      if (!matchings[i] && other < matchings.size() && matchings[other]) {
        matchings[i] = pos.getCoord();
        foundInactive = true;
        break;
      }
    }
    if (!foundInactive)
      matchings.push_back(pos.getCoord());
    recalculateDistances(pos.getLevel());
    return true;
  }
  return false;
}
