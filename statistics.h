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

#ifndef _STATISTICS_H
#define _STATISTICS_H

#include "util.h"
#include "enums.h"

enum class StatId {
  DEATH,
  CHOPPED_HEAD,
  CHOPPED_LIMB,
  INNOCENT_KILLED,
  SPELL_CAST,
  SCROLL_READ,
  ARMOR_PRODUCED,
  WEAPON_PRODUCED,
  POTION_PRODUCED,
};

ENUM_HASH(StatId);

class Statistics {
  public:
  static void init();
  static void add(StatId);
  static vector<string> getText();

  template <class Archive>
  static void serialize(Archive& ar, const unsigned int version);

  private:
  static unordered_map<StatId, int> count;
};

#endif
