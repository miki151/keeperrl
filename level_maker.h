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
#include "creature_factory.h"
#include "item_factory.h"
#include "square_factory.h"

enum class BuildingId { WOOD, MUD, BRICK, WOOD_CASTLE, DUNGEON};

class ItemFactory;
class CollectiveBuilder;
class LevelBuilder;
class StairKey;

class LevelGenException {
};

enum class SettlementType {
  VILLAGE,
  VILLAGE2,
  FOREST,
  CASTLE,
  CASTLE2,
  COTTAGE,
  TOWER,
  WITCH_HOUSE,
  MINETOWN,
  ANT_NEST,
  SMALL_MINETOWN,
  VAULT,
  CAVE,
  SPIDER_CAVE,
  ISLAND_VAULT,
  CEMETERY,
  SWAMP,
};

struct StockpileInfo {
  enum Type { GOLD, MINERALS } type;
  int number;
};

struct SettlementInfo {
  SettlementType type;
  optional<CreatureFactory> creatures;
  int numCreatures;
  optional<pair<CreatureFactory, int>> neutralCreatures;
  Location* location;
  Tribe* tribe;
  BuildingId buildingId;
  vector<StairKey> downStairs;
  vector<StairKey> upStairs;
  vector<StockpileInfo> stockpiles;
  optional<CreatureId> guardId;
  optional<ItemType> elderLoot;
  optional<ItemFactory> shopFactory;
  CollectiveBuilder* collective;
  optional<SquareFactory> furniture;
  optional<SquareFactory> outsideFeatures;
};

class LevelMaker {
  public:
  virtual void make(LevelBuilder* builder, Rectangle area) = 0;

  static LevelMaker* cryptLevel(RandomGen&, SettlementInfo);
  static LevelMaker* topLevel(RandomGen&, CreatureFactory forrest, vector<SettlementInfo> village);
  static LevelMaker* mineTownLevel(RandomGen&, SettlementInfo);
  static LevelMaker* splashLevel(CreatureFactory heroLeader, CreatureFactory heroes,
      CreatureFactory monsters, CreatureFactory imps, const string& splashPath);
  static LevelMaker* pyramidLevel(RandomGen&, optional<CreatureFactory>, vector<StairKey> up, vector<StairKey> down);
  static LevelMaker* towerLevel(RandomGen&, SettlementInfo);
  static Vec2 getRandomExit(RandomGen&, Rectangle rect, int minCornerDist = 1);
  static LevelMaker* roomLevel(RandomGen&, CreatureFactory roomFactory, CreatureFactory waterFactory,
    CreatureFactory lavaFactory, vector<StairKey> up, vector<StairKey> down, SquareFactory);
  static LevelMaker* mazeLevel(RandomGen&, SettlementInfo);
  static LevelMaker* quickLevel(RandomGen&);
};

#endif
