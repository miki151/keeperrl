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
#include "unique_entity.h"

class GameInfo;
class MapMemory;
class ViewIndex;
class Level;
class Position;

class CreatureView {
  public:
  virtual const MapMemory& getMemory() const = 0;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const = 0;
  virtual void refreshGameInfo(GameInfo&) const = 0;
  enum class CenterType {
    NONE,
    STAY_ON_SCREEN,
    FOLLOW
  };
  virtual CenterType getCenterType() const = 0;
  virtual Position getPosition() const = 0;
  virtual double getAnimationTime() const = 0;
  virtual vector<Vec2> getVisibleEnemies() const = 0;
  virtual const vector<Vec2>& getUnknownLocations(WConstLevel) const = 0;
  virtual ~CreatureView() {}
};

