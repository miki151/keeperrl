#include "stdafx.h"
#include "collective_teams.h"
#include "creature.h"
#include "team_order.h"

bool CollectiveTeams::contains(TeamId team, const Creature* c) const {
  return teamInfo.at(team).creatures.contains(c);
}

void CollectiveTeams::add(TeamId team, Creature* c) {
  if(!teamInfo[team].creatures.contains(c))
    teamInfo[team].creatures.push_back(c);
}

void CollectiveTeams::remove(TeamId team, Creature* c) {
  teamInfo[team].creatures.removeElement(c);
  if (teamInfo[team].creatures.empty())
    cancel(team);
}

void CollectiveTeams::activate(TeamId team) {
  teamInfo[team].active = true;
}

void CollectiveTeams::deactivate(TeamId team) {
  teamInfo[team].active = false;
}

void CollectiveTeams::setLeader(TeamId team, Creature* c) {
  if (!teamInfo[team].creatures.contains(c))
    add(team, c);
  swap(teamInfo[team].creatures[0], teamInfo[team].creatures[*teamInfo[team].creatures.findElement(c)]);
}

void CollectiveTeams::rotateLeader(TeamId team) {
  vector<Creature*> tmp = teamInfo.at(team).creatures;
  teamInfo.at(team).creatures.clear();
  for (int i = 1; i < tmp.size(); ++i)
    teamInfo.at(team).creatures.push_back(tmp[i]);
  teamInfo.at(team).creatures.push_back(tmp[0]);
}

Creature* CollectiveTeams::getLeader(TeamId team) const {
  CHECK(!teamInfo.at(team).creatures.empty());
  return teamInfo.at(team).creatures[0];
}

const vector<Creature*>& CollectiveTeams::getMembers(TeamId team) const {
  return teamInfo.at(team).creatures;
}

vector<TeamId> CollectiveTeams::getContaining(const Creature* c) const {
  vector<TeamId> ret;
  for (auto& team : teamInfo)
    if (contains(team.first, c))
      ret.push_back(team.first);
  return ret;
}

vector<TeamId> CollectiveTeams::getAll() const {
  return getKeys(teamInfo);
}

vector<TeamId> CollectiveTeams::getActive(const Creature* c) const {
  vector<TeamId> ret;
  for (TeamId t : getContaining(c))
    if (isActive(t))
      ret.push_back(t);
  return ret;
}

vector<TeamId> CollectiveTeams::getAllActive() const {
  vector<TeamId> ret;
  for (TeamId t : getKeys(teamInfo))
    if (isActive(t))
      ret.push_back(t);
  return ret;
}

TeamId CollectiveTeams::create(vector<Creature*> c) {
  CHECK(!c.empty());
  teamInfo[nextId].creatures = c;
  return nextId++;
}

bool CollectiveTeams::hasTeamOrder(TeamId id, const Creature* c, TeamOrder order) const {
  CHECK(exists(id));
  return teamInfo.at(id).teamOrders.count(order);
}

void CollectiveTeams::setTeamOrder(TeamId id, const Creature* c, TeamOrder order, bool state) {
  CHECK(exists(id));
  if (state) {
    teamInfo.at(id).teamOrders.insert(order);
    for (auto order2 : ENUM_ALL(TeamOrder))
      if (order != order2 && conflict(order, order2))
        teamInfo.at(id).teamOrders.erase(order2);
  } else
    teamInfo.at(id).teamOrders.erase(order);
}

bool CollectiveTeams::exists(TeamId id) const {
  return teamInfo.count(id);
}

bool CollectiveTeams::isActive(TeamId team) const {
  return teamInfo.at(team).active;
}

void CollectiveTeams::cancel(TeamId team) {
  teamInfo.erase(team);
}

SERIALIZE_DEF(CollectiveTeams, teamInfo, nextId)
