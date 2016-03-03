#ifndef _VISIBILITY_MAP_H
#define _VISIBILITY_MAP_H

#include "util.h"
#include "position_map.h"

class Creature;
class Level;

class VisibilityMap {
  public:
  void update(const Creature*, vector<Position> visibleTiles);
  void remove(const Creature*);
  bool isVisible(Position) const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  map<const Creature*, vector<Position>> SERIAL(lastUpdates);
  PositionMap<int> SERIAL(visibilityCount);
};

#endif

