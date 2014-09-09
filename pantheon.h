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
#include "singleton.h"
#include "creature_factory.h"

enum class DeityHabitat {
  EARTH,
  FIRE,
  AIR,
  TREES,
  WATER,
  STARS };

RICH_ENUM(EpithetId,
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
  LIGHT,
  LIGHTNING,
  LOVE,
  MIND,
  NATURE,
  SECRETS,
  WAR,
  WEALTH,
  WISDOM
);

enum class WorshipType {
  PRAYER,
  SACRIFICE,
  DESTROY_ALTAR,
};

class Epithet : public Singleton<Epithet, EpithetId> {
  public:
  const string& getName() const;
  const string& getDescription() const;

  static void init();

  private:
  Epithet(const string& name, const string& description);
  string name;
  string description;
};

class Deity {
  public:
  Deity(const string& name, Gender gender, const vector<EpithetId>& epithets, DeityHabitat habitat);
  const string& getName() const;
  Gender getGender() const;
  vector<EpithetId> getEpithets() const;
  string getEpithetsString() const;
  string getHabitatString() const;
  DeityHabitat getHabitat() const;
  CreatureFactory getServant(Tribe*) const;

  static Deity* getDeity(DeityHabitat);
  static Deity* getDeity(EpithetId);
  static vector<Deity*> getDeities();

  SERIALIZATION_DECL(Deity);

  template <class Archive>
  static void serializeAll(Archive& ar) {
    ar & deities & byEpithet;
  }

  private:
  static vector<Deity*> deities;
  static EnumMap<EpithetId, Deity*> byEpithet;
  string SERIAL(name);
  Gender SERIAL(gender);
  vector<EpithetId> SERIAL(epithets);
  DeityHabitat SERIAL(habitat);
};

#endif
