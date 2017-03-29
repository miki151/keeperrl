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
#include "level.h"
#include "collective_teams.h"
#include "tribe.h"
#include "effect_type.h"
#include "task.h"
#include "collective_attack.h"
#include "territory.h"
#include "game.h"
#include "collective_name.h"
#include "lasting_effect.h"
#include "body.h"
#include "event_proxy.h"
#include "attack_trigger.h"
#include "immigration.h"

typedef EnumVariant<AttackTriggerId, TYPES(int),
        ASSIGN(int, AttackTriggerId::ENEMY_POPULATION, AttackTriggerId::GOLD)> OldTrigger;

SERIALIZATION_CONSTRUCTOR_IMPL(VillageControl);

VillageControl::VillageControl(Collective* col, optional<VillageBehaviour> v) : CollectiveControl(col),
    eventProxy(this, col->getModel()), villain(v) {
  for (Position v : col->getTerritory().getAll())
    for (WItem it : v.getItems())
      myItems.insert(it);
}

Collective* VillageControl::getEnemyCollective() const {
  return getCollective()->getGame()->getPlayerCollective();
}

bool VillageControl::isEnemy(const Creature* c) {
  if (Collective* col = getEnemyCollective())
    return contains(col->getCreatures(), c);
  else
    return false;
}

void VillageControl::onOtherKilled(const Creature* victim, const Creature* killer) {
  if (victim->getTribe() == getCollective()->getTribe())
    if (isEnemy(killer))
      victims += 0.15; // small increase for same tribe but different village
}

void VillageControl::onMemberKilled(const Creature* victim, const Creature* killer) {
  if (isEnemy(killer))
    victims += 1;
}

void VillageControl::onEvent(const GameEvent& event) {
  switch (event.getId()) {
    case EventId::PICKED_UP: {
      auto info = event.get<EventInfo::ItemsHandled>();
      if (getCollective()->getTerritory().contains(info.creature->getPosition()))
        if (isEnemy(info.creature) && villain)
          if (contains(villain->triggers, AttackTriggerId::STOLEN_ITEMS)) {
            bool wasTheft = false;
            for (const WItem it : info.items)
              if (myItems.contains(it)) {
                wasTheft = true;
                ++stolenItemCount;
                myItems.erase(it);
              }
            if (getCollective()->hasLeader() && wasTheft) {
              info.creature->playerMessage(PlayerMessage("You are going to regret this", MessagePriority::HIGH));
            }
          }
    }
    break;
    default:
    break;
  }
}

void VillageControl::launchAttack(vector<Creature*> attackers) {
  if (Collective* enemy = getEnemyCollective()) {
    for (Creature* c : attackers)
//      if (getCollective()->getGame()->canTransferCreature(c, enemy->getLevel()->getModel()))
        getCollective()->getGame()->transferCreature(c, enemy->getModel());
    optional<int> ransom;
    int hisGold = enemy->numResource(CollectiveResourceId::GOLD);
    if (villain->ransom && hisGold >= villain->ransom->second)
      ransom = max<int>(villain->ransom->second,
          (Random.getDouble(villain->ransom->first * 0.6, villain->ransom->first * 1.5)) * hisGold);
    enemy->addAttack(CollectiveAttack(getCollective(), attackers, ransom));
    TeamId team = getCollective()->getTeams().createPersistent(attackers);
    getCollective()->getTeams().activate(team);
    getCollective()->freeTeamMembers(team);
    for (Creature* c : attackers)
      getCollective()->setTask(c, villain->getAttackTask(this));
    attackSizes[team] = attackers.size();
  }
}

void VillageControl::considerCancellingAttack() {
  for (auto team : getCollective()->getTeams().getAll()) {
    vector<Creature*> members = getCollective()->getTeams().getMembers(team);
    if (members.size() < (attackSizes[team] + 1) / 2 || (members.size() == 1 &&
          members[0]->getBody().isSeriouslyWounded())) {
      for (Creature* c : members)
        getCollective()->cancelTask(c);
      getCollective()->getTeams().cancel(team);
    }
  }
}

void VillageControl::onRansomPaid() {
  for (auto team : getCollective()->getTeams().getAll()) {
    vector<Creature*> members = getCollective()->getTeams().getMembers(team);
    for (Creature* c : members)
      getCollective()->cancelTask(c);
    getCollective()->getTeams().cancel(team);
  }
}

vector<TriggerInfo> VillageControl::getTriggers(const Collective* against) const {
  vector<TriggerInfo> ret;
  if (villain && against == getEnemyCollective())
    for (auto& elem : villain->triggers)
      ret.push_back({elem, villain->getTriggerValue(elem, this)});
  return ret;
}

void VillageControl::considerWelcomeMessage() {
  if (!getCollective()->hasLeader())
    return;
  if (villain)
    if (villain->welcomeMessage)
      switch (*villain->welcomeMessage) {
        case VillageBehaviour::DRAGON_WELCOME:
          for (Position pos : getCollective()->getTerritory().getAll())
            if (Creature* c = pos.getCreature())
              if (c->isAffected(LastingEffect::INVISIBLE) && isEnemy(c) && c->isPlayer()
                  && getCollective()->getLeader()->canSee(c->getPosition())) {
                c->playerMessage(PlayerMessage("\"Well thief! I smell you and I feel your air. "
                      "I hear your breath. Come along!\"", MessagePriority::CRITICAL));
                villain->welcomeMessage.reset();
              }
          break;
      }
}

void VillageControl::checkEntries() {
  if (villain)
    for (auto& trigger : villain->triggers)
      if (trigger.getId() == AttackTriggerId::ENTRY)
        for (Position pos : getCollective()->getTerritory().getAll())
          if (Creature* c = pos.getCreature())
            if (getCollective()->getTribe()->isEnemy(c))
              entries = true;
}

bool VillageControl::canPerformAttack(bool currentlyActive) {
  return !currentlyActive ||
      getCollective()->getModel() == getCollective()->getGame()->getMainModel().get();
}

void VillageControl::acceptImmigration() {
  for (int i : All(getCollective()->getConfig().getImmigrantInfo()))
    getCollective()->getImmigration().setAutoState(i, ImmigrantAutoState::AUTO_ACCEPT);
}

void VillageControl::update(bool currentlyActive) {
  considerWelcomeMessage();
  considerCancellingAttack();
  checkEntries();
  acceptImmigration();
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
  if (canPerformAttack(currentlyActive) && Random.roll(1 / updateFreq))
    if (villain) {
      if (Collective* enemy = getEnemyCollective())
        maxEnemyPower = max(maxEnemyPower, enemy->getDangerLevel());
      double prob = villain->getAttackProbability(this) / updateFreq;
      if (Random.chance(prob)) {
        vector<Creature*> fighters;
        fighters = getCollective()->getCreatures({MinionTrait::FIGHTER}, {MinionTrait::SUMMONED});
        /*if (getCollective()->getGame()->isSingleModel())
          fighters = filter(fighters, [this] (const Creature* c) {
              return contains(getCollective()->getTerritory().getAll(), c->getPosition()); });*/
        INFO << getCollective()->getName().getShort() << " fighters: " << int(fighters.size())
          << (!getCollective()->getTeams().getAll().empty() ? " attacking " : "");
        if (fighters.size() >= villain->minTeamSize && 
            allMembers.size() >= villain->minPopulation + villain->minTeamSize)
        launchAttack(getPrefix(Random.permutation(fighters),
          Random.get(villain->minTeamSize, min(fighters.size(), allMembers.size() - villain->minPopulation) + 1)));
      }
    }
}

