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
#include "spell_school_id.h"

class View;
class Technology;
class GameConfig;
class BuildInfo;
class SpellSchool;
class Spell;
struct PlayerInfo;
struct SpellSchoolInfo;
struct ItemInfo;
class ContentFactory;
class Workshops;

class Encyclopedia {
  public:
  Encyclopedia(ContentFactory*);
  ~Encyclopedia();

  void setKeeperThings(ContentFactory*, const Technology*, const Workshops*);
  const Technology* technology;
  vector<SpellSchoolInfo> spellSchools;
  vector<PlayerInfo> bestiary;
  vector<ItemInfo> items;
};

