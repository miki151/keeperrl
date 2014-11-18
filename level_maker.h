/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _LEVEL_MAKER_H
#define _LEVEL_MAKER_H

#include "util.h"
#include "level.h"
#include "creature_factory.h"
#include "item_factory.h"

enum class BuildingId { WOOD, MUD, BRICK, WOOD_CASTLE, DUNGEON};

class ItemFactory;
class CollectiveBuilder;

enum class SettlementType {
  VILLAGE,
  VILLAGE2,
  CASTLE,
  CASTLE2,
  COTTAGE,
  WITCH_HOUSE,
  MINETOWN,
  SMALL_MINETOWN,
  VAULT,
  CAVE,
  ISLAND_VAULT,
};

struct StockpileInfo {
  enum Type { GOLD, MINERALS } type;
  int number;
};

struct SettlementInfo {
  SettlementType type;
  CreatureFactory creatures;
  int numCreatures;
  Optional<pair<CreatureFactory, int>> neutralCreatures;
  Location* location;
  Tribe* tribe;
  BuildingId buildingId;
  vector<StairKey> downStairs;
  vector<StairKey> upStairs;
  vector<StockpileInfo> stockpiles;
  Optional<CreatureId> guardId;
  Optional<ItemType> elderLoot;
  Optional<ItemFactory> shopFactory;
  CollectiveBuilder* collective;
};

class LevelMaker {
  public:
  virtual void make(Level::Builder* builder, Rectangle area) = 0;

  static LevelMaker* roomLevel(CreatureFactory roomFactory, CreatureFactory waterFactory,
    CreatureFactory fireFactory, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* cryptLevel(CreatureFactory roomFactory, CreatureFactory::SingleCreature coffinFactory,
      vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* cellarLevel(CreatureFactory cfactory, SquareType wallType, StairLook stairLook,
      vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* cavernLevel(CreatureFactory cfactory, SquareType wallType, SquareType floorType,
      StairLook stairLook, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* topLevel(CreatureFactory forrest, vector<SettlementInfo> village);
  static LevelMaker* mineTownLevel(SettlementInfo);

  static LevelMaker* pyramidLevel(Optional<CreatureFactory>, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* towerLevel(Optional<StairKey> down, Optional<StairKey> up);
  static Vec2 getRandomExit(Rectangle rect, int minCornerDist = 1);
  static LevelMaker* grassAndTrees();
};

#endif
