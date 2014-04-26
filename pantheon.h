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

#ifndef _PANTHEON_H
#define _PANTHEON_H

#include "util.h"
#include "gender.h"

enum class DeityHabitat {
  EARTH,
  STONE,
  FIRE,
  AIR,
  TREES,
  WATER,
  STARS };

enum class Epithet {
  CHANGE,
  COURAGE,
  CRAFTS,
  DARKNESS,
  DEATH,
  DEFENSE,
  DESTRUCTION,
  FEAR,
  FORTUNE,
  HEALTH,
  HUNTING,
  LIGHT,
  LIGHTNING,
  LOVE,
  MIND,
  NATURE,
  SECRETS,
  WAR,
  WEALTH,
  WINTER,
  WISDOM,
};

class Deity {
  public:
  Deity(const string& name, Gender gender, const vector<Epithet>& epithets, DeityHabitat habitat);
  string getName() const;
  Gender getGender() const;
  string getEpithets() const;
  string getHabitatString() const;
  DeityHabitat getHabitat() const;
  void onPrayer(Creature* c);

  static Deity* getDeity(DeityHabitat);
  static vector<Deity*> getDeities();

  SERIALIZATION_DECL(Deity);

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar & deities;
  }

  private:
  static vector<Deity*> deities;
  Deity(Deity&) {}
  string SERIAL(name);
  Gender SERIAL(gender);
  vector<Epithet> SERIAL(epithets);
  vector<Epithet> SERIAL(usedEpithets);
  DeityHabitat SERIAL(habitat);
};

#endif
