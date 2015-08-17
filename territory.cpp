#include "stdafx.h"
#include "territory.h"
#include "position.h"
#include "movement_type.h"

template <class Archive>
void Territory::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(allSquares) & SVAR(allSquaresVec);
}

SERIALIZABLE(Territory);

void Territory::clearCache() {
  extendedCache.clear();
  extendedCache2.clear();
}

void Territory::insert(Position pos) {
  allSquaresVec.push_back(pos);
  allSquares.insert(pos);
  clearCache();
}

void Territory::remove(Position pos) {
  removeElement(allSquaresVec, pos);
  allSquares.erase(pos);
  clearCache();
}
  
bool Territory::contains(Position pos) const {
  return allSquares.count(pos);
}

const vector<Position>& Territory::getAll() const {
  return allSquaresVec;
}

vector<Position> Territory::calculateExtended(int minRadius, int maxRadius) const {
  map<Position, int> extendedTiles;
  vector<Position> extendedQueue;
  for (Position pos : allSquaresVec) {
    if (!extendedTiles.count(pos))  {
      extendedTiles[pos] = 1;
      extendedQueue.push_back(pos);
    }
  }
  for (int i = 0; i < extendedQueue.size(); ++i) {
    Position pos = extendedQueue[i];
    for (Position v : pos.neighbors8())
      if (!contains(v) && !extendedTiles.count(v) 
          && v.canEnterEmpty({MovementTrait::WALK})) {
        int a = extendedTiles[v] = extendedTiles[pos] + 1;
        if (a < maxRadius)
          extendedQueue.push_back(v);
      }
  }
  if (minRadius > 0)
    return filter(extendedQueue, [&] (const Position& v) { return extendedTiles.at(v) >= minRadius; });
  return extendedQueue;
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

