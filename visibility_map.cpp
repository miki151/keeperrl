#include "stdafx.h"
#include "visibility_map.h"
#include "creature.h"
#include "creature_name.h"


template <class Archive> 
void VisibilityMap::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, lastUpdates, visibilityCount);
}

SERIALIZABLE(VisibilityMap);

void VisibilityMap::update(const Creature* c, vector<Position> visibleTiles) {
  remove(c);
  lastUpdates.set(c, visibleTiles);
  for (Position v : visibleTiles)
    ++visibilityCount.getOrInit(v);
}

void VisibilityMap::remove(const Creature* c) {
  if (auto pos = lastUpdates.getMaybe(c))
    for (Position v : *pos)
      --visibilityCount.getOrFail(v);
  lastUpdates.erase(c);
}

bool VisibilityMap::isVisible(Position pos) const {
  return visibilityCount.get(pos) > 0;
}

