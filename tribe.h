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

#ifndef _TRIBE_H
#define _TRIBE_H

#include <map>
#include <set>

#include "enums.h"
#include "event.h"
#include "singleton.h"

class Creature;

class Tribe {
  public:
  double getStanding(const Creature*) const;
  bool isEnemy(const Creature*) const;
  bool isEnemy(const Tribe*) const;
  void makeSlightEnemy(const Creature*);
  void addMember(const Creature*);
  void removeMember(const Creature*);
  const string& getName() const;
  void addEnemy(vector<Tribe*>);
  void addFriend(Tribe*);

  void onMemberKilled(Creature* member, Creature* killer);
  void onMemberAttacked(Creature* member, Creature* attacker);
  void onItemsStolen(const Creature* thief);

  SERIALIZATION_DECL(Tribe);

  struct Set {
    Set();

    PTribe SERIAL(monster);
    PTribe SERIAL(pest);
    PTribe SERIAL(wildlife);
    PTribe SERIAL(human);
    PTribe SERIAL(elven);
    PTribe SERIAL(dwarven);
    PTribe SERIAL(adventurer);
    PTribe SERIAL(bandit);
    PTribe SERIAL(killEveryone);
    PTribe SERIAL(peaceful);
    PTribe SERIAL(keeper);
    PTribe SERIAL(lizard);

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
  };

  friend struct Set;

  private:
  Tribe(const string& name, bool diplomatic);

  bool SERIAL(diplomatic);

  void initStanding(const Creature*);
  double getMultiplier(const Creature* member);

  unordered_map<const Creature*, double> SERIAL(standing);
  vector<pair<Creature*, Creature*>> SERIAL(attacks);
  vector<const Creature*> SERIAL(members);
  unordered_set<Tribe*> SERIAL(enemyTribes);
  string SERIAL(name);
};

#endif
