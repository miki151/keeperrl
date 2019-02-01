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

#include "spell_id.h"
#include "game_time.h"

class Spell;

class SpellMap {
  public:
  void add(Spell*);
  void add(SpellId);
  GlobalTime getReadyTime(Spell*) const;
  void setReadyTime(Spell*, GlobalTime);
  vector<Spell*> getAll() const;
  bool contains(Spell*) const;
  bool contains(SpellId) const;
  void clear();
  void onExpLevelReached(Creature*, double level);
  void setAllReady();

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EnumMap<SpellId, optional<GlobalTime>> SERIAL(elems);
};

