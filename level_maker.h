#ifndef _LEVEL_MAKER_H
#define _LEVEL_MAKER_H

#include "util.h"
#include "level.h"
#include "creature_factory.h"

enum class BuildingId { WOOD, MUD, BRICK };

struct SettlementInfo {
  SettlementType type;
  CreatureFactory factory;
  int numCreatures;
  Optional<CreatureId> elder;
  Location* location;
  Tribe* tribe;
  pair<int, int> size;
  BuildingId buildingId;
  vector<StairKey> downStairs;
  Optional<CreatureId> guardId;
  Optional<ItemId> elderLoot;
  Optional<ItemFactory> shopFactory;
};

class LevelMaker {
  public:
  virtual void make(Level::Builder* builder, Rectangle area) = 0;

  static LevelMaker* roomLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* cryptLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* cellarLevel(CreatureFactory cfactory, SquareType wallType, StairLook stairLook,
      vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* cavernLevel(CreatureFactory cfactory, SquareType wallType, SquareType floorType,
      StairLook stairLook, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* topLevel(CreatureFactory forrest, vector<SettlementInfo> village);
  static LevelMaker* topLevel2(CreatureFactory forrest, vector<SettlementInfo> village);
  static LevelMaker* mineTownLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* goblinTownLevel(CreatureFactory cfactory, vector<StairKey> up, vector<StairKey> down);

  static LevelMaker* pyramidLevel(Optional<CreatureFactory>, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* towerLevel(Optional<StairKey> down, Optional<StairKey> up);
  static Vec2 getRandomExit(Rectangle rect, int minCornerDist = 1);
  static LevelMaker* collectiveLevel(vector<StairKey> up, vector<StairKey> down, StairKey hellDown, Collective* col);
  static LevelMaker* grassAndTrees();
};

#endif
