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
#include "t_string.h"

SERIALIZE_DEF(Statistics, count)

void Statistics::add(StatId id) {
  ++count[id];
}

void Statistics::clear() {
  for (StatId id : ENUM_ALL(StatId))
    count[id] = 0;
}

static TStringId getName(StatId id) {
  switch (id) {
    case StatId::DEATH: return TStringId("DEATHS");
    case StatId::INNOCENT_KILLED: return TStringId("COLD_BLOODED_MURDERS");
    case StatId::CHOPPED_HEAD: return TStringId("CHOPPED_HEADS");
    case StatId::CHOPPED_LIMB: return TStringId("CHOPPED_LIMBS");
    case StatId::SPELL_CAST: return TStringId("SPELLS_CAST");
    case StatId::SCROLL_READ: return TStringId("SCROLLS_READ");
    case StatId::WEAPON_PRODUCED: return TStringId("WEAPONS_PRODUCED");
    case StatId::ARMOR_PRODUCED: return TStringId("PIECES_OF_ARMOR_PRODUCED");
    case StatId::POTION_PRODUCED: return TStringId("POTIONS_PRODUCED");
  }
}

vector<TString> Statistics::getText() const {
  vector<TString> ret;
  for (auto id : ENUM_ALL(StatId)) {
    if (int n = count[id])
      ret.emplace_back(TSentence(getName(id), TString(n)));
  }
  return ret;
}

