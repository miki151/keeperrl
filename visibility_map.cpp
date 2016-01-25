#include "stdafx.h"
#include "visibility_map.h"
#include "creature.h"
#include "entity_name.h"

SERIALIZATION_CONSTRUCTOR_IMPL(VisibilityMap);

template <class Archive> 
void VisibilityMap::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(lastUpdates) & SVAR(visibilityCount);
}

SERIALIZABLE(VisibilityMap);

VisibilityMap::VisibilityMap(const vector<Level*>& levels) : visibilityCount(levels) {
}

void VisibilityMap::update(const Creature* c, vector<Position> visibleTiles) {
  remove(c);
  lastUpdates[c] = visibleTiles;
  for (Position v : visibleTiles)
    ++visibilityCount[v];
}

void VisibilityMap::remove(const Creature* c) {
  for (Position v : lastUpdates[c])
    --visibilityCount[v];
  lastUpdates.erase(c);
}

bool VisibilityMap::isVisible(Position pos) const {
  return visibilityCount[pos] > 0;
}

