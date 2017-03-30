#pragma once

#include "util.h"
#include "position_map.h"
#include "unique_entity.h"
#include "entity_map.h"

class Creature;
class Level;

class VisibilityMap {
  public:
  void update(WConstCreature, vector<Position> visibleTiles);
  void remove(WConstCreature);
  bool isVisible(Position) const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  EntityMap<Creature, vector<Position>> SERIAL(lastUpdates);
  PositionMap<int> SERIAL(visibilityCount);
};


