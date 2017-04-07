#include "stdafx.h"
#include "visibility_map.h"
#include "creature.h"
#include "creature_name.h"


template <class Archive> 
void VisibilityMap::serialize(Archive& ar, const unsigned int version) {
  ar(lastUpdates, visibilityCount);
}

SERIALIZABLE(VisibilityMap);

void VisibilityMap::update(WConstCreature c, vector<Position> visibleTiles) {
  remove(c);
  lastUpdates.set(c, visibleTiles);
  for (Position v : visibleTiles)
    if (++visibilityCount.getOrInit(v) == 1)
      v.setNeedsRenderUpdate(true);
}

void VisibilityMap::remove(WConstCreature c) {
  if (auto pos = lastUpdates.getMaybe(c))
    for (Position v : *pos)
      if (--visibilityCount.getOrFail(v) == 0)
      v.setNeedsRenderUpdate(true);
  lastUpdates.erase(c);
}

bool VisibilityMap::isVisible(Position pos) const {
  return visibilityCount.get(pos) > 0;
}

