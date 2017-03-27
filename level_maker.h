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

#pragma once

#include "util.h"
#include "creature_factory.h"
#include "item_factory.h"
#include "square_factory.h"
#include "furniture_factory.h"
#include "item_type.h"

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
  FORREST_COTTAGE,
  TOWER,
  WITCH_HOUSE,
  MINETOWN,
  ANT_NEST,
  SMALL_MINETOWN,
  VAULT,
  CAVE,
  SPIDER_CAVE,
  ISLAND_VAULT,
  ISLAND_VAULT_DOOR,
  CEMETERY,
  SWAMP,
  MOUNTAIN_LAKE,
};

RICH_ENUM(BiomeId,
  GRASSLAND,
  FORREST,
  MOUNTAIN
);

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
  TribeId tribe;
  optional<string> race;
  BuildingId buildingId;
  vector<StairKey> downStairs;
  vector<StairKey> upStairs;
  vector<StockpileInfo> stockpiles;
  optional<CreatureId> guardId;
  optional<ItemType> elderLoot;
  optional<ItemFactory> shopFactory;
  CollectiveBuilder* collective;
  optional<FurnitureFactory> furniture;
  optional<FurnitureFactory> outsideFeatures;
  bool closeToPlayer;
};

class LevelMaker {
  public:
  virtual void make(LevelBuilder* builder, Rectangle area) = 0;

  static PLevelMaker cryptLevel(RandomGen&, SettlementInfo);
  static PLevelMaker topLevel(RandomGen&, CreatureFactory forrest, vector<SettlementInfo> village, int width,
      bool keeperSpawn, BiomeId);
  static PLevelMaker mineTownLevel(RandomGen&, SettlementInfo);
  static PLevelMaker splashLevel(CreatureFactory heroLeader, CreatureFactory heroes,
      CreatureFactory monsters, CreatureFactory imps, const FilePath& splashPath);
  static PLevelMaker towerLevel(RandomGen&, SettlementInfo);
  static Vec2 getRandomExit(RandomGen&, Rectangle rect, int minCornerDist = 1);
  static PLevelMaker roomLevel(RandomGen&, CreatureFactory roomFactory, CreatureFactory waterFactory,
    CreatureFactory lavaFactory, vector<StairKey> up, vector<StairKey> down, FurnitureFactory furniture);
  static PLevelMaker mazeLevel(RandomGen&, SettlementInfo);
  static PLevelMaker quickLevel(RandomGen&);
  static PLevelMaker emptyLevel(RandomGen&);
  static PLevelMaker sokobanFromFile(RandomGen&, SettlementInfo, Table<char>);
};

