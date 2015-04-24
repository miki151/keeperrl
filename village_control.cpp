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


#include "stdafx.h"
#include "village_control.h"
#include "collective.h"
#include "creature.h"
#include "square.h"

template <class Archive>
void VillageControl::Villain::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(minPopulation)
    & SVAR(minTeamSize)
    & SVAR(collective)
    & SVAR(triggers)
    & SVAR(behaviour)
    & SVAR(leaderAttacks)
    & SVAR(attackMessage);
  if (version == 1) { // OBSOLETE
    ar & SVAR(itemTheftMessage)
       & SVAR(welcomeMessage);
  }
}

VillageControl::VillageControl(Collective* col, const Location* l, vector<Villain> v)
    : CollectiveControl(col), location(l), villains(v) {
  for (Vec2 v : l->getBounds()) {
    getCollective()->claimSquare(v);
    for (Item* it : getCollective()->getLevel()->getSafeSquare(v)->getItems())
      myItems.insert(it);
  }
}

optional<VillageControl::Villain&> VillageControl::getVillain(const Creature* c) {
  for (auto& villain : villains)
    if (villain.contains(c))
      return villain;
  return none;
}

void VillageControl::onKillEvent(const Creature* victim, const Creature* killer) {
  if (victim->getTribe() == getCollective()->getTribe())
    if (auto villain = getVillain(killer)) {
      if (contains(getCollective()->getCreatures(), victim))
        victims[villain->collective] += 1;
      else
        victims[villain->collective] += 0.15; // small increase for same tribe but different village
    }
}

void VillageControl::onPickupEvent(const Creature* who, const vector<Item*>& items) {
  if (who->getPosition().inRectangle(location->getBounds()))
    if (auto villain = getVillain(who)) {
      bool wasTheft = false;
      for (const Item* it : items)
        if (myItems.contains(it)) {
          wasTheft = true;
          ++stolenItemCount[villain->collective];
          myItems.erase(it);
        }
      if (getCollective()->getLeader() && wasTheft && villain->itemTheftMessage) {
        switch (*villain->itemTheftMessage) {
          case DRAGON_THEFT:
            who->playerMessage(PlayerMessage("You are going to regret this", PlayerMessage::HIGH));
            break;
        }
        villain->itemTheftMessage.reset();
      }
    }
}

void VillageControl::launchAttack(Villain& villain, vector<Creature*> attackers) {
  Debug() << getAttackMessage(villain, attackers);
  villain.collective->addAssaultNotification(getCollective(), attackers, getAttackMessage(villain, attackers));
  TeamId team = getCollective()->getTeams().create(attackers);
  getCollective()->getTeams().activate(team);
  getCollective()->freeTeamMembers(team);
  for (Creature* c : attackers)
    getCollective()->setTask(c, villain.getAttackTask(this));
}

void VillageControl::considerWelcomeMessage() {
  if (Creature* leader = getCollective()->getLeader())
    for (auto& villain : villains)
      if (villain.welcomeMessage)
        switch (*villain.welcomeMessage) {
          case DRAGON_WELCOME:
            for (Vec2 pos : location->getBounds())
              if (Creature* c = getCollective()->getLevel()->getSafeSquare(pos)->getCreature())
                if (c->isAffected(LastingEffect::INVISIBLE) && villain.contains(c) && c->isPlayer()
                    && leader->canSee(c->getPosition())) {
                  c->playerMessage(PlayerMessage("\"Well thief! I smell you and I feel your air. "
                        "I hear your breath. Come along!\"", PlayerMessage::HIGH));
                  villain.welcomeMessage.reset();
                }
            break;
        }
}

void VillageControl::tick(double time) {
  considerWelcomeMessage();
  vector<Creature*> allMembers = getCollective()->getCreatures();
  for (auto team : getCollective()->getTeams().getAll()) {
    for (const Creature* c : getCollective()->getTeams().getMembers(team))
      if (!getCollective()->hasTask(c)) {
        getCollective()->getTeams().cancel(team);
        break;
      }
    return;
  }
  double updateFreq = 0.1;
  if (Random.roll(1 / updateFreq))
    for (auto& villain : villains) {
      vector<Creature*> fighters;
      if (villain.leaderAttacks)
        fighters = getCollective()->getCreatures({MinionTrait::FIGHTER});
      else
        fighters = getCollective()->getCreatures({MinionTrait::FIGHTER}, {MinionTrait::LEADER});
      Debug() << getCollective()->getName() << " fighters: " << int(fighters.size())
        << (!getCollective()->getTeams().getAll().empty() ? " attacking " : "");
      if (fighters.size() < villain.minTeamSize || allMembers.size() < villain.minPopulation + villain.minTeamSize)
        continue;
      double prob = villain.getAttackProbability(this) / updateFreq;
      if (prob > 0 && Random.roll(1 / prob)) {
        launchAttack(villain, getPrefix(randomPermutation(fighters),
              Random.get(villain.minTeamSize, min(fighters.size(), allMembers.size() - villain.minPopulation) + 1)));
        break;
      }
    }
}

MoveInfo VillageControl::getMove(Creature* c) {
  return NoMove;
}

PTask VillageControl::Villain::getAttackTask(VillageControl* self) {
  switch (behaviour.getId()) {
    case VillageBehaviourId::KILL_LEADER: return Task::attackLeader(collective);
    case VillageBehaviourId::KILL_MEMBERS: return Task::killFighters(collective, behaviour.get<int>());
    case VillageBehaviourId::STEAL_GOLD: return Task::stealFrom(collective, self->getCollective());
  }
  assert("__FUNCTION__ case unknown behaviour id");
  return Task::attackLeader(collective);
}

string VillageControl::getAttackMessage(const Villain& villain, const vector<Creature*> attackers) const {
  switch (villain.attackMessage) {
    case CREATURE_TITLE:
      return "You are under attack by " + attackers[0]->getNameAndTitle() + "!";
    case TRIBE_AND_NAME:
      if (getCollective()->getName().empty())
        return "You are under attack by " + getCollective()->getTribe()->getName() + "!";
      else
        return "You are under attack by " + getCollective()->getTribe()->getName() + 
            " of " + getCollective()->getName() + "!";
  }
  return "";
}

static double powerClosenessFun(double myPower, double hisPower) {
  if (myPower == 0 || hisPower == 0)
    return 0;
  double a = myPower / hisPower;
  double valueAt2 = 0.5;
  if (a < 0.4)
    return 0;
  if (a < 1)
    return a * a * a; // fast growth close to 1
  else if (a < 2)
    return 1.0 - (a - 1) * (a - 1) * (a - 1) * valueAt2; // slow descent close to 1, valueAt2 at 2
  else
    return valueAt2 / (a - 1); // converges to 0, valueAt2 at 2
}

static double victimsFun(int victims, int minPopulation) {
  if (!victims)
    return 0;
  else if (victims == 1)
    return 0.1;
  else if (victims == 2)
    return 0.3;
  else
    return 1.0;
}

static double populationFun(int population, int minPopulation) {
  double diff = double(population - minPopulation) / minPopulation;
  if (diff < 0)
    return 0;
  else if (diff < 0.1)
    return 0.1;
  else if (diff < 0.2)
    return 0.3;
  else if (diff < 0.33)
    return 0.6;
  else
    return 1.0;
}

static double goldFun(int gold, int minGold) {
  double diff = double(gold - minGold) / minGold;
  if (diff < 0)
    return 0;
  else if (diff < 0.1)
    return 0.1;
  else if (diff < 0.4)
    return 0.3;
  else if (diff < 1)
    return 0.6;
  else
    return 1.0;
}

static double stolenItemsFun(int numStolen) {
  if (!numStolen)
    return 0;
  else 
    return 1.0;
}

bool VillageControl::Villain::contains(const Creature* c) {
  return ::contains(collective->getCreatures(), c);
}

double VillageControl::Villain::getTriggerValue(const Trigger& trigger, const VillageControl* self,
    const Collective* villain) const {
  double powerMaxProb = 1.0 / 10000; // rather small chance that they attack just because you are strong
  double victimsMaxProb = 1.0 / 500;
  double populationMaxProb = 1.0 / 500;
  double goldMaxProb = 1.0 / 500;
  double stolenMaxProb = 1.0 / 300;
  switch (trigger.getId()) {
    case AttackTriggerId::POWER: 
      return powerMaxProb * powerClosenessFun(self->getCollective()->getWarLevel(), villain->getWarLevel());
    case AttackTriggerId::SELF_VICTIMS:
      return victimsMaxProb * victimsFun(self->victims.count(villain) ? self->victims.at(villain) : 0, 0);
    case AttackTriggerId::ENEMY_POPULATION:
      return populationMaxProb * populationFun(
          villain->getCreatures(MinionTrait::FIGHTER).size(), trigger.get<int>());
    case AttackTriggerId::GOLD:
      return goldMaxProb * goldFun(villain->numResource(Collective::ResourceId::GOLD), trigger.get<int>());
    case AttackTriggerId::STOLEN_ITEMS:
      return stolenMaxProb 
          * stolenItemsFun(self->stolenItemCount.count(villain) ? self->stolenItemCount.at(villain) : 0);
  }
  return 0;
}

double VillageControl::Villain::getAttackProbability(const VillageControl* self) const {
  double ret = 0;
  for (auto& elem : triggers) {
    double val = getTriggerValue(elem, self, collective);
    CHECK(val >= 0 && val <= 1);
    ret = max(ret, val);
    Debug() << "trigger " << int(elem.getId()) << " village " << self->getCollective()->getTribe()->getName()
      << " under attack probability " << val;
  }
  return ret;
}

SERIALIZATION_CONSTRUCTOR_IMPL(VillageControl);

template <class Archive>
void VillageControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(CollectiveControl)
    & SVAR(location)
    & SVAR(villains)
    & SVAR(victims);
  if (version == 1) {
    ar & SVAR(myItems)
       & SVAR(stolenItemCount);
  }
}

SERIALIZABLE(VillageControl);
