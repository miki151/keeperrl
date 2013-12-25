#ifndef _LEVEL_MAKER_H
#define _LEVEL_MAKER_H

#include "util.h"
#include "level.h"
#include "creature_factory.h"


class LevelMaker {
  public:
  virtual void make(Level::Builder* builder, Rectangle area) = 0;

  static LevelMaker* roomLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* cryptLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* hellLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* topLevel(
      CreatureFactory forrest,
      vector<CreatureFactory> village,
      vector<Location*> villageLocation);
  static LevelMaker* mineTownLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* goblinTownLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);

  static LevelMaker* pyramidLevel(Optional<CreatureFactory>, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* towerLevel(Optional<StairKey> down, Optional<StairKey> up);
  static Vec2 getRandomExit(Rectangle rect);

  static LevelMaker* collectiveLevel(vector<StairKey> up, vector<StairKey> down, StairKey hellDown, Collective* col);
};

#endif
