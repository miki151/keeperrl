#include "stdafx.h"
#include "collective_teams.h"
#include "creature.h"

bool CollectiveTeams::contains(TeamId team, WConstCreature c) const {
  return teamInfo.at(team).creatures.contains(c);
}

void CollectiveTeams::add(TeamId team, WCreature c) {
  CHECK(!teamInfo[team].creatures.contains(c));
  teamInfo[team].creatures.push_back(c);
}

void CollectiveTeams::remove(TeamId team, WCreature c) {
  if (c == getLeader(team))
    deactivate(team); // otherwise teams are still active when the player gets killed
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

void CollectiveTeams::setLeader(TeamId team, WCreature c) {
  if (!teamInfo[team].creatures.contains(c))
    add(team, c);
  swap(teamInfo[team].creatures[0], teamInfo[team].creatures[*teamInfo[team].creatures.findElement(c)]);
}

void CollectiveTeams::rotateLeader(TeamId team) {
  vector<WCreature> tmp = teamInfo.at(team).creatures;
  teamInfo.at(team).creatures.clear();
  for (int i = 1; i < tmp.size(); ++i)
    teamInfo.at(team).creatures.push_back(tmp[i]);
  teamInfo.at(team).creatures.push_back(tmp[0]);
}

WCreature CollectiveTeams::getLeader(TeamId team) const {
  CHECK(!teamInfo.at(team).creatures.empty());
  return teamInfo.at(team).creatures[0];
}

const vector<WCreature>& CollectiveTeams::getMembers(TeamId team) const {
  return teamInfo.at(team).creatures;
}

vector<WCreature> CollectiveTeams::getMembers(TeamId team) {
  return teamInfo.at(team).creatures;
}

vector<TeamId> CollectiveTeams::getContaining(WConstCreature c) const {
  vector<TeamId> ret;
  for (auto team : getKeys(teamInfo))
    if (contains(team, c))
      ret.push_back(team);
  return ret;
}

vector<TeamId> CollectiveTeams::getAll() const {
  return getKeys(teamInfo);
}

vector<TeamId> CollectiveTeams::getActive(WConstCreature c) const {
  vector<TeamId> ret;
  for (TeamId t : getContaining(c))
    if (isActive(t))
      ret.push_back(t);
  return ret;
}

vector<TeamId> CollectiveTeams::getActiveNonPersistent(WConstCreature c) const {
  vector<TeamId> ret;
  for (TeamId t : getContaining(c))
    if (isActive(t) && !isPersistent(t))
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

TeamId CollectiveTeams::create(vector<WCreature> c) {
  CHECK(!c.empty());
  teamInfo[nextId].creatures = c;
  return nextId++;
}

TeamId CollectiveTeams::createPersistent(vector<WCreature> c) {
  TeamId id = create(c);
  teamInfo[id].persistent = true;
  return id;
}

bool CollectiveTeams::isPersistent(TeamId id) const {
  CHECK(exists(id));
  return teamInfo.at(id).persistent;
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
