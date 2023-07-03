#include "stdafx.h"
#include "territory.h"
#include "position.h"
#include "movement_type.h"
#include "position_map.h"

SERIALIZE_DEF(Territory, allSquares, allSquaresVec, centralPoint)

void Territory::clearCache() {
  extendedCache.clear();
  extendedCache2.clear();
}

void Territory::insert(Position pos) {
  if (!allSquares.count(pos)) {
    allSquaresVec.push_back(pos);
    allSquares.insert(pos);
    clearCache();
  }
}

void Territory::remove(Position pos) {
  allSquaresVec.removeElement(pos);
  allSquares.erase(pos);
  clearCache();
}

void Territory::setCentralPoint(Position pos) {
  centralPoint = pos;
}

bool Territory::contains(Position pos) const {
  return allSquares.count(pos);
}

const vector<Position>& Territory::getAll() const {
  return allSquaresVec;
}

const PositionSet& Territory::getAllAsSet() const {
  return allSquares;
}

vector<Position> Territory::calculateExtended(int minRadius, int maxRadius) const {
  PROFILE;
  PositionMap<int> extendedTiles;
  vector<Position> extendedQueue;
  for (Position pos : allSquaresVec) {
    if (!extendedTiles.contains(pos))  {
      extendedTiles.set(pos, 1);
      extendedQueue.push_back(pos);
    }
  }
  for (int i = 0; i < extendedQueue.size(); ++i) {
    Position pos = extendedQueue[i];
    auto value = extendedTiles.getOrFail(pos);
    for (Position v : pos.neighbors8())
      if (!contains(v) && !extendedTiles.contains(v) && v.canEnterEmpty({MovementTrait::WALK})) {
        extendedTiles.set(v, value + 1);
        if (value + 1 < maxRadius)
          extendedQueue.push_back(v);
      }
  }
  if (minRadius > 0)
    return extendedQueue.filter([&] (const Position& v) { return extendedTiles.getOrFail(v) >= minRadius; });
  return extendedQueue;
}

const vector<Position>& Territory::getStandardExtended() const {
  return getExtended(2, 10);
}

const vector<Position>& Territory::getExtended(int min, int max) const {
  if (!extendedCache.count(make_pair(min, max)))
    extendedCache[make_pair(min, max)] = calculateExtended(min, max);
  return extendedCache.at(make_pair(min, max));
}

const vector<Position>& Territory::getExtended(int max) const {
  if (!extendedCache2.count(max))
    extendedCache2[max] = calculateExtended(0, max);
  return extendedCache2.at(max);
}

bool Territory::isEmpty() const {
  return allSquaresVec.empty();
}

const optional<Position>& Territory::getCentralPoint() const {
  return centralPoint;
}

IterateVectors<Position> Territory::getPillagePositions() const {
  return iterateVectors(getAll(), getStandardExtended());
}