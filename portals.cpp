#include "stdafx.h"
#include "portals.h"
#include "shortest_path.h"
#include "level.h"

SERIALIZE_DEF(Portals, matchings, distanceToNearest)
SERIALIZATION_CONSTRUCTOR_IMPL(Portals)

optional<int> Portals::getPortalIndex(Position position) const {
  return matchings.findElement(position).map([](int a) {return a / 2;});
}

void Portals::removePortal(Position position) {
  if (auto index = matchings.findElement(position)) {
    matchings[*index] = none;
    recalculateDistances(position.getLevel());
  }
}

optional<short> Portals::getDistanceToNearest(Position pos) const {
  if (auto table = getReferenceMaybe(distanceToNearest, pos.getLevel()->getUniqueId()))
    return (*table)[pos.getCoord()];
  return none;
}

void Portals::recalculateDistances(Level* level) {
  vector<Vec2> portals;
  for (auto& portal : matchings)
    if (portal && portal->getLevel() == level)
      portals.push_back(portal->getCoord());
  auto ret = Table<optional<short>>(level->getBounds());
  auto entryFun = [&](Vec2 pos) {
    if (Position(pos, level).canEnterEmpty({MovementTrait::WALK}))
      return 1;
    else
      return 10000;
  };
  Dijkstra dijkstra(ret.getBounds(), portals, 10000, entryFun);
  for (auto& pos : dijkstra.getAllReachable())
    ret[pos.first] = (short) pos.second;
  distanceToNearest.insert(make_pair(level->getUniqueId(), std::move(ret)));
}

int getOtherIndex(int index) {
  return index - index % 2 + (index + 1) % 2;
}

optional<Position> Portals::getOtherPortal(Position position) const {
  if (auto index = matchings.findElement(position)) {
    auto otherIndex = getOtherIndex(*index);
    if (otherIndex < matchings.size())
      return matchings[otherIndex];
  }
  return none;
}

bool Portals::registerPortal(Position pos) {
  if (!matchings.contains(pos)) {
    bool foundInactive = false;
    for (int i : All(matchings)) {
      int other = getOtherIndex(i);
      if (!matchings[i] && other < matchings.size() && matchings[other]) {
        matchings[i] = pos;
        foundInactive = true;
        break;
      }
    }
    if (!foundInactive)
      matchings.push_back(pos);
    recalculateDistances(pos.getLevel());
    return true;
  }
  return false;
}
