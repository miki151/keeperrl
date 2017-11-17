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

#include "stdafx.h"
#include "statistics.h"

SERIALIZE_DEF(Statistics, count)

void Statistics::add(StatId id) {
  ++count[id];
}

void Statistics::clear() {
  for (StatId id : ENUM_ALL(StatId))
    count[id] = 0;
}

vector<pair<StatId, string>> text {
  {StatId::DEATH, "deaths"}, 
  {StatId::INNOCENT_KILLED, "cold blooded murders"}, 
  {StatId::CHOPPED_HEAD, "chopped heads" },
  {StatId::CHOPPED_LIMB, "chopped limbs" },
  {StatId::SPELL_CAST, "spells cast" },
  {StatId::SCROLL_READ, "scrolls read" },
  {StatId::WEAPON_PRODUCED, "weapons produced" },
  {StatId::ARMOR_PRODUCED, "pieces of armor produced" },
  {StatId::POTION_PRODUCED, "potions produced" },

};

vector<string> Statistics::getText() const {
  vector<string> ret;
  for (auto elem : text) {
    if (int n = count[elem.first])
      ret.emplace_back(toString(n) + " " + elem.second);
  }
  return ret;
}

