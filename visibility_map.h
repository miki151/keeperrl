#ifndef _VISIBILITY_MAP_H
#define _VISIBILITY_MAP_H

#include "util.h"

class Creature;

class VisibilityMap {
  public:
  VisibilityMap(Rectangle bounds);
  void update(const Creature*, vector<Vec2> visibleTiles);
  void remove(const Creature*);
  bool isVisible(Vec2 pos) const;

  SERIALIZATION_DECL(VisibilityMap);

  private:
  map<const Creature*, vector<Vec2>> SERIAL(lastUpdates);
  Table<int> SERIAL(visibilityCount);
};

#endif

