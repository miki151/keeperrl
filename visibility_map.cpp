#include "stdafx.h"
#include "visibility_map.h"
#include "creature.h"


SERIALIZATION_CONSTRUCTOR_IMPL(VisibilityMap);

template <class Archive> 
void VisibilityMap::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(lastUpdates) & SVAR(visibilityCount);
}

SERIALIZABLE(VisibilityMap);

VisibilityMap::VisibilityMap(Rectangle bounds) : visibilityCount(bounds, 0) {
}

void VisibilityMap::update(const Creature* c, vector<Vec2> visibleTiles) {
  remove(c);
  lastUpdates[c] = visibleTiles;
  for (Vec2 v : visibleTiles)
    ++visibilityCount[v];
}

void VisibilityMap::remove(const Creature* c) {
  for (Vec2 v : lastUpdates[c]) {
    CHECK(v.inRectangle(visibilityCount.getBounds())) << v << " " << c->getPosition() << " " << c->getName().bare();
    --visibilityCount[v];
  }
  lastUpdates.erase(c);
}

bool VisibilityMap::isVisible(Vec2 pos) const {
  return visibilityCount[pos] > 0;
}

