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
#include "enums.h"

RICH_ENUM(StatId,
  DEATH,
  CHOPPED_HEAD,
  CHOPPED_LIMB,
  INNOCENT_KILLED,
  SPELL_CAST,
  SCROLL_READ,
  ARMOR_PRODUCED,
  WEAPON_PRODUCED,
  POTION_PRODUCED
);

class TString;

class Statistics {
  public:
  void add(StatId);
  vector<TString> getText() const;
  void clear();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EnumMap<StatId, int> SERIAL(count);
};

