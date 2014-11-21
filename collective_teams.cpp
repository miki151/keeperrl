#include "stdafx.h"
#include "collective_teams.h"
#include "creature.h"

bool CollectiveTeams::contains(TeamId team, const Creature* c) const {
  return ::contains(teamInfo.at(team).creatures(), c);
}

void CollectiveTeams::add(TeamId team, Creature* c) {
  CHECK(!::contains(teamInfo[team].creatures(), c));
  teamInfo[team].creatures().push_back(c);
}

void CollectiveTeams::remove(TeamId team, Creature* c) {
  removeElement(teamInfo[team].creatures(), c);
}

void CollectiveTeams::activate(TeamId team) {
  teamInfo[team].active() = true;
}

void CollectiveTeams::deactivate(TeamId team) {
  teamInfo[team].active() = false;
}

void CollectiveTeams::setLeader(TeamId team, Creature* c) {
  if (!::contains(teamInfo[team].creatures(), c))
    add(team, c);
  swap(teamInfo[team].creatures()[0], teamInfo[team].creatures()[*findElement(teamInfo[team].creatures(), c)]);
}

const Creature* CollectiveTeams::getLeader(TeamId team) const {
  CHECK(!teamInfo.at(team).creatures().empty());
  return teamInfo.at(team).creatures()[0];
}

Creature* CollectiveTeams::getLeader(TeamId team) {
  CHECK(!teamInfo.at(team).creatures().empty());
  return teamInfo.at(team).creatures()[0];
}

const vector<Creature*>& CollectiveTeams::getMembers(TeamId team) const {
  return teamInfo.at(team).creatures();
}

vector<Creature*> CollectiveTeams::getMembers(TeamId team) {
  return teamInfo.at(team).creatures();
}

vector<TeamId> CollectiveTeams::getContaining(const Creature* c) const {
  vector<TeamId> ret;
  for (auto team : getKeys(teamInfo))
    if (contains(team, c))
      ret.push_back(team);
  return ret;
}

vector<TeamId> CollectiveTeams::getAll() const {
  return getKeys(teamInfo);
}

vector<TeamId> CollectiveTeams::getActiveTeams(const Creature* c) const {
  vector<TeamId> ret;
  for (TeamId t : getContaining(c))
    if (isActive(t))
      ret.push_back(t);
  return ret;
}

vector<TeamId> CollectiveTeams::getActiveTeams() const {
  vector<TeamId> ret;
  for (TeamId t : getKeys(teamInfo))
    if (isActive(t))
      ret.push_back(t);
  return ret;
}

TeamId CollectiveTeams::create(vector<Creature*> c) {
  static int cnt = 0;
  TeamId id = ++cnt;
  teamInfo[id].creatures() = c;
  return id;
}

TeamId CollectiveTeams::createHidden(vector<Creature*> c) {
  TeamId id = create(c);
  teamInfo[id].hidden() = true;
  return id;
}

bool CollectiveTeams::isHidden(TeamId id) const {
  CHECK(exists(id));
  return teamInfo.at(id).hidden();
}

bool CollectiveTeams::exists(TeamId id) const {
  return teamInfo.count(id);
}

bool CollectiveTeams::isActive(TeamId team) const {
  return teamInfo.at(team).active();
}

void CollectiveTeams::cancel(TeamId team) {
  teamInfo.erase(team);
}

template <class Archive>
void CollectiveTeams::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(teamInfo);
}

SERIALIZABLE(CollectiveTeams);
