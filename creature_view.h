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
#include "view_id.h"
#include "view_object.h"

class GameInfo;
class MapMemory;
class ViewIndex;
class Level;
class Position;

enum class CreatureViewCenterType {
  NONE,
  STAY_ON_SCREEN,
  FOLLOW
};

class CreatureView {
  public:
  using CenterType = CreatureViewCenterType;
  virtual const MapMemory& getMemory() const = 0;
  virtual void getViewIndex(Vec2 pos, ViewIndex&) const = 0;
  virtual void refreshGameInfo(GameInfo&) const = 0;
  virtual CenterType getCenterType() const = 0;
  virtual Vec2 getScrollCoord() const = 0;
  virtual Level* getCreatureViewLevel() const = 0;
  virtual double getAnimationTime() const = 0;
  virtual vector<Vec2> getVisibleEnemies() const = 0;
  virtual const vector<Vec2>& getUnknownLocations(WConstLevel) const = 0;
  virtual optional<Vec2> getSelectionSize() const { return none; }
  virtual vector<vector<Vec2>> getPathTo(UniqueEntity<Creature>::Id, Vec2) const { return {}; }
  virtual vector<vector<Vec2>> getGroupPathTo(const string& group, Vec2) const { return {}; }
  virtual vector<vector<Vec2>> getTeamPathTo(TeamId, Vec2) const { return {}; }
  virtual vector<Vec2> getHighlightedPathTo(Vec2) const { return {}; }
  virtual vector<vector<Vec2>> getPermanentPaths() const { return {}; }
  virtual ~CreatureView() {}
};

