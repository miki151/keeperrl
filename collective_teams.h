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
#ifndef _COLLECTIVE_TEAMS_H
#define _COLLECTIVE_TEAMS_H

#include "util.h"
#include "enums.h"

class Creature;

class CollectiveTeams {
  public:
  bool contains(TeamId, const Creature*) const;
  bool isActive(TeamId) const;
  void add(TeamId, Creature*);
  void remove(TeamId, Creature*);
  void setLeader(TeamId, Creature*);
  void activate(TeamId);
  void deactivate(TeamId);
  TeamId create(vector<Creature*>);
  TeamId createHidden(vector<Creature*>);
  const Creature* getLeader(TeamId) const;
  Creature* getLeader(TeamId);
  const vector<Creature*>& getMembers(TeamId) const;
  vector<Creature*> getMembers(TeamId);
  vector<TeamId> getContaining(const Creature*) const;
  vector<TeamId> getAll() const;
  vector<TeamId> getActiveTeams(const Creature*) const;
  vector<TeamId> getActiveTeams() const;
  void cancel(TeamId);
  bool exists(TeamId) const;
  bool isHidden(TeamId) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  struct TeamInfo : public NamedTupleBase<vector<Creature*>, bool, bool> {
    NAMED_TUPLE_STUFF(TeamInfo);
    NAME_ELEM(0, creatures);
    NAME_ELEM(1, active);
    NAME_ELEM(2, hidden);
  };
  map<TeamId, TeamInfo> SERIAL(teamInfo);
  TeamId SERIAL(nextId) = 1;
};

#endif
