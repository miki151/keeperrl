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


class ItemFactory;
class CollectiveBuilder;
class LevelBuilder;
class StairKey;
class FurnitureFactory;
class CreatureGroup;
struct SettlementInfo;

class LevelGenException {
};

class FilePath;
class CreatureList;
class TribeId;

class LevelMaker {
  public:
  virtual void make(LevelBuilder* builder, Rectangle area) = 0;
  virtual ~LevelMaker() {}

  static PLevelMaker cryptLevel(RandomGen&, SettlementInfo);
  static PLevelMaker topLevel(RandomGen&, optional<CreatureGroup> wildlife, vector<SettlementInfo> village, int width,
      optional<TribeId> keeperTribe, BiomeId);
  static PLevelMaker mineTownLevel(RandomGen&, SettlementInfo);
  static PLevelMaker splashLevel(CreatureGroup heroLeader, CreatureGroup heroes,
      CreatureGroup monsters, CreatureGroup imps, const FilePath& splashPath);
  static PLevelMaker towerLevel(RandomGen&, SettlementInfo);
  static Vec2 getRandomExit(RandomGen&, Rectangle rect, int minCornerDist = 1);
  static PLevelMaker roomLevel(RandomGen&, CreatureGroup roomFactory, CreatureGroup waterFactory,
    CreatureGroup lavaFactory, vector<StairKey> up, vector<StairKey> down, FurnitureFactory furniture);
  static PLevelMaker mazeLevel(RandomGen&, SettlementInfo);
  static PLevelMaker emptyLevel(FurnitureType);
  static PLevelMaker sokobanFromFile(RandomGen&, SettlementInfo, Table<char>);
  static PLevelMaker battleLevel(Table<char>, CreatureList allies, CreatureList enemies);
  static PLevelMaker getFullZLevel(RandomGen&, optional<SettlementInfo>, int mapWidth, TribeId keeperTribe, StairKey);
  static PLevelMaker getWaterZLevel(RandomGen&, FurnitureType waterType, int mapWidth, CreatureList, StairKey);
};

