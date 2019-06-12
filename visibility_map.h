#pragma once

#include "util.h"
#include "position_map.h"
#include "unique_entity.h"
#include "entity_map.h"

class Creature;
class Level;

class VisibilityMap {
  public:
  vector<Position> update(const Creature*, const vector<Position>& visibleTiles);
  void remove(const Creature*);
  void updateEyeball(Position);
  void removeEyeball(Position);
  void onVisibilityChanged(Position);
  bool isVisible(Position) const;

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  EntityMap<Creature, vector<Position>> SERIAL(lastUpdates);
  PositionMap<vector<Position>> SERIAL(eyeballs);
  PositionMap<int> SERIAL(visibilityCount);
  vector<Position> addPositions(const vector<Position>&);
  void removePositions(const vector<Position>&);
};


