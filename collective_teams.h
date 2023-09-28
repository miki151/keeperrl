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
#include "unique_entity.h"

class Creature;

class CollectiveTeams {
  public:
  bool contains(TeamId, const Creature*) const;
  bool isActive(TeamId) const;
  void add(TeamId, Creature*);
  void remove(TeamId, Creature*);
  void setLeader(TeamId, Creature*);
  void rotateLeader(TeamId);
  void activate(TeamId);
  void deactivate(TeamId);
  TeamId create(vector<Creature*>);
  Creature* getLeader(TeamId) const;
  const vector<Creature*>& getMembers(TeamId) const;
  vector<TeamId> getContaining(const Creature*) const;
  vector<TeamId> getAll() const;
  vector<TeamId> getActive(const Creature*) const;
  vector<TeamId> getAllActive() const;
  void cancel(TeamId);
  bool exists(TeamId) const;
  bool hasTeamOrder(TeamId, TeamOrder) const;
  void setTeamOrder(TeamId, TeamOrder, bool);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  struct TeamInfo {
    vector<Creature*> SERIAL(creatures);
    set<TeamOrder> SERIAL(teamOrders);
    bool SERIAL(active);
    SERIALIZE_ALL(creatures, teamOrders, active);
  };
  map<TeamId, TeamInfo> SERIAL(teamInfo);
  TeamId SERIAL(nextId) = 1;
};
