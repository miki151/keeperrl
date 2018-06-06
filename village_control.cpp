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
#include "effect.h"
#include "task.h"
#include "collective_attack.h"
#include "territory.h"
#include "game.h"
#include "collective_name.h"
#include "lasting_effect.h"
#include "body.h"
#include "attack_trigger.h"
#include "immigration.h"
#include "village_behaviour.h"
#include "furniture.h"

typedef EnumVariant<AttackTriggerId, TYPES(int),
        ASSIGN(int, AttackTriggerId::ENEMY_POPULATION, AttackTriggerId::GOLD)> OldTrigger;

SERIALIZATION_CONSTRUCTOR_IMPL(VillageControl)

SERIALIZE_DEF(VillageControl, SUBCLASS(CollectiveControl), SUBCLASS(EventListener), villain, victims, myItems, stolenItemCount, attackSizes, entries, maxEnemyPower)
REGISTER_TYPE(ListenerTemplate<VillageControl>)

VillageControl::VillageControl(Private, WCollective col, optional<VillageBehaviour> v) : CollectiveControl(col),
    villain(v) {
  for (Position v : col->getTerritory().getAll())
    for (WItem it : v.getItems())
      myItems.insert(it);
}

PVillageControl VillageControl::create(WCollective col, optional<VillageBehaviour> v) {
  auto ret = makeOwner<VillageControl>(Private{}, col, v);
  ret->subscribeTo(col->getModel());
  return ret;
}

WCollective VillageControl::getEnemyCollective() const {
  return collective->getGame()->getPlayerCollective();
}

bool VillageControl::isEnemy(WConstCreature c) {
  if (WCollective col = getEnemyCollective())
    return col->getCreatures().contains(c);
  else
    return false;
}

void VillageControl::onOtherKilled(WConstCreature victim, WConstCreature killer) {
  if (victim->getTribe() == collective->getTribe())
    if (isEnemy(killer))
      victims += 0.15; // small increase for same tribe but different village
}

void VillageControl::onMemberKilled(WConstCreature victim, WConstCreature killer) {
  if (isEnemy(killer))
    victims += 1;
}

void VillageControl::onEvent(const GameEvent& event) {
  using namespace EventInfo;
  event.visit(
      [&](const ItemsPickedUp& info) {
        if (!collective->isConquered() && collective->getTerritory().contains(info.creature->getPosition()))
          if (isEnemy(info.creature) && villain)
            if (villain->triggers.contains(AttackTriggerId::STOLEN_ITEMS)) {
              bool wasTheft = false;
              for (WConstItem it : info.items)
                if (myItems.contains(it)) {
                  wasTheft = true;
                  ++stolenItemCount;
                  myItems.erase(it);
                }
              if (wasTheft) {
                info.creature->privateMessage(PlayerMessage("You are going to regret this", MessagePriority::HIGH));
              }
            }
      },
      [&](const FurnitureDestroyed& info) {
        if (collective->getTerritory().contains(info.position) && Furniture::isWall(info.type))
          for (auto neighbor : info.position.neighbors8())
            if (auto c = neighbor.getCreature())
              if (isEnemy(c)) {
                entries = true;
                break;
              }
      },
      [&](const auto&) {}
  );
}

void VillageControl::launchAttack(vector<WCreature> attackers) {
  if (WCollective enemy = getEnemyCollective()) {
    for (WCreature c : attackers)
//      if (getCollective()->getGame()->canTransferCreature(c, enemy->getLevel()->getModel()))
        collective->getGame()->transferCreature(c, enemy->getModel());
    optional<int> ransom;
    int hisGold = enemy->numResource(CollectiveResourceId::GOLD);
    if (villain->ransom && hisGold >= villain->ransom->second)
      ransom = max<int>(villain->ransom->second,
          (Random.getDouble(villain->ransom->first * 0.6, villain->ransom->first * 1.5)) * hisGold);
    TeamId team = collective->getTeams().createPersistent(attackers);
    collective->getTeams().activate(team);
    collective->freeTeamMembers(attackers);
    vector<WConstTask> attackTasks;
    for (WCreature c : attackers) {
      PTask task;
      if (c != collective->getTeams().getLeader(team))
        task = Task::chain(Task::follow(c), villain->getAttackTask(this));
      else
        task = villain->getAttackTask(this);
      attackTasks.push_back(task.get());
      collective->setTask(c, std::move(task));
    }
    enemy->addAttack(CollectiveAttack(std::move(attackTasks), collective, attackers, ransom));
    attackSizes[team] = attackers.size();
  }
}

void VillageControl::considerCancellingAttack() {
  for (auto team : collective->getTeams().getAll()) {
    vector<WCreature> members = collective->getTeams().getMembers(team);
    if (members.size() < (attackSizes[team] + 1) / 2 || (members.size() == 1 &&
          members[0]->getBody().isSeriouslyWounded())) {
      for (WCreature c : members)
        collective->freeFromTask(c);
      collective->getTeams().cancel(team);
    }
  }
}

void VillageControl::onRansomPaid() {
  for (auto team : collective->getTeams().getAll()) {
    vector<WCreature> members = collective->getTeams().getMembers(team);
    for (WCreature c : members)
      collective->freeFromTask(c);
    collective->getTeams().cancel(team);
  }
}

vector<TriggerInfo> VillageControl::getTriggers(WConstCollective against) const {
  vector<TriggerInfo> ret;
  if (villain && against == getEnemyCollective())
    for (auto& elem : villain->triggers)
      ret.push_back({elem, villain->getTriggerValue(elem, this)});
  return ret;
}

void VillageControl::considerWelcomeMessage() {
  PROFILE;
  auto leader = collective->getLeader();
  if (!leader)
    return;
  if (villain)
    if (villain->welcomeMessage)
      switch (*villain->welcomeMessage) {
        case VillageBehaviour::DRAGON_WELCOME:
          for (Position pos : collective->getTerritory().getAll())
            if (WCreature c = pos.getCreature())
              if (c->isAffected(LastingEffect::INVISIBLE) && isEnemy(c) && c->isPlayer()
                  && leader->canSee(c->getPosition())) {
                c->privateMessage(PlayerMessage("\"Well thief! I smell you and I feel your air. "
                      "I hear your breath. Come along!\"", MessagePriority::CRITICAL));
                villain->welcomeMessage.reset();
              }
          break;
      }
}

bool VillageControl::canPerformAttack(bool currentlyActive) {
  return !currentlyActive ||
      collective->getModel() == collective->getGame()->getMainModel().get();
}

void VillageControl::acceptImmigration() {
  for (int i : All(collective->getConfig().getImmigrantInfo()))
    collective->getImmigration().setAutoState(i, ImmigrantAutoState::AUTO_ACCEPT);
}

void VillageControl::healAllCreatures() {
  for (auto c : collective->getCreatures())
    c->heal(0.002);
}

void VillageControl::update(bool currentlyActive) {
  considerWelcomeMessage();
  considerCancellingAttack();
  acceptImmigration();
  healAllCreatures();
  vector<WCreature> allMembers = collective->getCreatures();
  for (auto team : collective->getTeams().getAll()) {
    for (WConstCreature c : collective->getTeams().getMembers(team))
      if (!collective->hasTask(c)) {
        collective->getTeams().cancel(team);
        break;
      }
    return;
  }
  double updateFreq = 0.1;
  if (canPerformAttack(currentlyActive) && Random.chance(updateFreq))
    if (villain) {
      if (WCollective enemy = getEnemyCollective())
        maxEnemyPower = max(maxEnemyPower, enemy->getDangerLevel());
      double prob = villain->getAttackProbability(this) / updateFreq;
      if (Random.chance(prob)) {
        vector<WCreature> fighters;
        fighters = collective->getCreatures(MinionTrait::FIGHTER);
        /*if (getCollective()->getGame()->isSingleModel())
          fighters = filter(fighters, [this] (WConstCreature c) {
              return contains(getCollective()->getTerritory().getAll(), c->getPosition()); });*/
        /*if (auto& name = collective->getName())
          INFO << name->shortened << " fighters: " << int(fighters.size())
            << (!collective->getTeams().getAll().empty() ? " attacking " : "");*/
        if (fighters.size() >= villain->minTeamSize && 
            allMembers.size() >= villain->minPopulation + villain->minTeamSize)
        launchAttack(getPrefix(Random.permutation(fighters),
          Random.get(villain->minTeamSize, min(fighters.size(), allMembers.size() - villain->minPopulation) + 1)));
      }
    }
}

