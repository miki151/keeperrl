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
#include "furniture_list.h"

class ItemFactory;
class CollectiveBuilder;
class LevelBuilder;
class StairKey;
class FurnitureFactory;
class CreatureGroup;
struct SettlementInfo;
struct ResourceCounts;

class LevelGenException {
};

class FilePath;
class CreatureList;
class TribeId;
class ContentFactory;;
struct BiomeInfo;
class Position;
struct KeeperBaseInfo;

class LevelMaker {
  public:
  virtual void make(LevelBuilder* builder, Rectangle area) = 0;
  virtual ~LevelMaker() {}

  static PLevelMaker topLevel(RandomGen&, vector<SettlementInfo> village, int width, int difficulty,
      optional<TribeId> keeperTribe, optional<KeeperBaseInfo>, BiomeInfo, ResourceCounts, const ContentFactory&);
  static PLevelMaker mineTownLevel(RandomGen&, SettlementInfo, Vec2 size, int difficulty);
  static PLevelMaker towerLevel(RandomGen&, SettlementInfo, Vec2 size, int difficulty);
  static Vec2 getRandomExit(RandomGen&, Rectangle rect, int minCornerDist = 1);
  static PLevelMaker roomLevel(RandomGen&, SettlementInfo, Vec2 size, int difficulty);
  static PLevelMaker mazeLevel(RandomGen&, SettlementInfo, Vec2 size, int difficulty);
  static PLevelMaker blackMarket(RandomGen&, SettlementInfo, Vec2 size, int difficulty);
  static PLevelMaker emptyLevel(FurnitureType, bool withFloor);
  static PLevelMaker upLevel(Position, const BiomeInfo&, vector<SettlementInfo>, optional<ResourceCounts>);
  static PLevelMaker sokobanFromFile(RandomGen&, SettlementInfo, Table<char>, int difficulty);
  static PLevelMaker battleLevel(Table<char>, vector<PCreature> allies, vector<CreatureList> enemies);
  static PLevelMaker getFullZLevel(RandomGen&, optional<SettlementInfo>, ResourceCounts, int mapWidth, TribeId keeperTribe,
      const ContentFactory&, int difficulty);
  static PLevelMaker getWaterZLevel(RandomGen&, FurnitureType waterType, int mapWidth, CreatureList);
  static PLevelMaker settlementLevel(const ContentFactory&, RandomGen&, SettlementInfo, Vec2 size,
      optional<ResourceCounts> resources, optional<TribeId> resourceTribe, FurnitureType mountainType, int difficulty);
};

