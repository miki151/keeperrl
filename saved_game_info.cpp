#include "stdafx.h"
#include "saved_game_info.h"
#include "game.h"
#include "collective.h"
#include "creature.h"
#include "creature_attributes.h"

SERIALIZE_DEF(SavedGameInfo, minions, dangerLevel, name);

SERIALIZATION_CONSTRUCTOR_IMPL(SavedGameInfo);

static SavedGameInfo::MinionInfo getInfo(const Creature* c) {
  SavedGameInfo::MinionInfo ret;
  ret.level = c->getAttributes().getExpLevel();
  ret.viewId = c->getViewObject().id();
  return ret;
}

SavedGameInfo::SavedGameInfo(const Game* g) {
  Collective* col = g->getPlayerCollective();
  dangerLevel = col->getDangerLevel();
  vector<Creature*> creatures = col->getCreatures();
  CHECK(!creatures.empty());
  Creature* leader = col->getLeader();
  CHECK(!leader->isDead());
  sort(creatures.begin(), creatures.end(), [leader] (const Creature* c1, const Creature* c2) {
    return c1 == leader || (c2 != leader && c1->getAttributes().getExpLevel() > c2->getAttributes().getExpLevel());});
  CHECK(creatures[0] == leader);
  creatures.resize(min<int>(creatures.size(), 4));
  for (Creature* c : creatures)
    minions.push_back(getInfo(c));
  name = *leader->getName().first();
}

const vector<SavedGameInfo::MinionInfo>& SavedGameInfo::getMinions() const {
  return minions;
}

double SavedGameInfo::getDangerLevel() const {
  return dangerLevel;
}

const string& SavedGameInfo::getName() const {
  return name;
}

ViewId SavedGameInfo::getViewId() const {
  return minions.at(0).viewId;
}

