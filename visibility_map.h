#ifndef _VISIBILITY_MAP_H
#define _VISIBILITY_MAP_H

#include "util.h"
#include "position_map.h"

class Creature;
class Level;

class VisibilityMap {
  public:
  VisibilityMap(const vector<Level*>&);
  void update(const Creature*, vector<Position> visibleTiles);
  void remove(const Creature*);
  bool isVisible(Position) const;

  SERIALIZATION_DECL(VisibilityMap);

  private:
  map<const Creature*, vector<Position>> SERIAL(lastUpdates);
  PositionMap<int> SERIAL(visibilityCount);
};

#endif

