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

RICH_ENUM(TribeId,
  MONSTER,
  PEST,
  WILDLIFE,
  HUMAN,
  ELVEN,
  DWARVEN,
  ORC,
  PLAYER,
  DRAGON,
  CASTLE_CELLAR,
  BANDIT,
  KILL_EVERYONE,
  PEACEFUL,
  KEEPER,
  LIZARD
);

class Creature;

class Tribe : public Singleton<Tribe, TribeId> {
  public:
  virtual double getStanding(const Creature*) const;

  void onItemsStolen(const Creature* thief);
  void makeSlightEnemy(const Creature*);
  void addMember(const Creature*);
  void removeMember(const Creature*);
  void setLeader(const Creature*);
  const Creature* getLeader() const;
  vector<const Creature*> getMembers(bool includeDead = false);
  const string& getName() const;
  void addEnemy(Tribe*);
  void addFriend(Tribe*);
  int getHandicap() const;
  void setHandicap(int);

  SERIALIZATION_DECL(Tribe);

  static void init();

  template <class Archive>
  static void registerTypes(Archive& ar);

  protected:
  Tribe(const string& name, bool diplomatic);

  private:
  REGISTER_HANDLER(KillEvent, const Creature* victim, const Creature* killer);
  REGISTER_HANDLER(AttackEvent, Creature* victim, Creature* attacker);

  bool SERIAL(diplomatic);

  void initStanding(const Creature*);
  double getMultiplier(const Creature* member);

  unordered_map<const Creature*, double> SERIAL(standing);
  vector<pair<Creature*, Creature*>> SERIAL(attacks);
  const Creature* SERIAL2(leader, nullptr);
  vector<const Creature*> SERIAL(members);
  unordered_set<Tribe*> SERIAL(enemyTribes);
  string SERIAL(name);
  int SERIAL2(handicap, 0);
};

#endif
