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
#include "creature_attributes.h"
#include "game_event.h"
#include "content_factory.h"
#include "enemy_aggression_level.h"
#include "immigrant_info.h"
#include "item.h"
#include "team_order.h"
#include "duel_state.h"

SERIALIZATION_CONSTRUCTOR_IMPL(VillageControl)

SERIALIZE_DEF(VillageControl, SUBCLASS(CollectiveControl), SUBCLASS(EventListener), behaviour, victims, myItems, stolenItemCount, attackSizes, entries, maxEnemyPower, cancelledAttacks)
REGISTER_TYPE(ListenerTemplate<VillageControl>)

VillageControl::VillageControl(Private, Collective* col, optional<VillageBehaviour> v) : CollectiveControl(col),
    behaviour(to_heap_optional(v)) {
  for (Position v : col->getTerritory().getAll())
    for (Item* it : v.getItems())
      myItems.insert(it);
}

PVillageControl VillageControl::create(Collective* col, optional<VillageBehaviour> v) {
  auto ret = makeOwner<VillageControl>(Private{}, col, v);
  ret->subscribeTo(col->getModel());
  return ret;
}

PVillageControl VillageControl::copyOf(Collective* col, const VillageControl* control) {
  optional<VillageBehaviour> b;
  if (control->behaviour)
    b = *control->behaviour;
  auto ret = makeOwner<VillageControl>(Private{}, col, std::move(b));
  ret->subscribeTo(col->getModel());
  return ret;
}

Collective* VillageControl::getEnemyCollective() const {
  return collective->getGame()->getPlayerCollective();
}

bool VillageControl::isEnemy(const Creature* c) {
  if (Collective* col = getEnemyCollective())
    return col->getCreatures().contains(c) && !col->hasTrait(c, MinionTrait::DOESNT_TRIGGER);
  else
    return false;
}

void VillageControl::onOtherKilled(const Creature* victim, const Creature* killer) {
  if (victim->getTribe() == collective->getTribe())
    if (isEnemy(killer))
      victims += 0.15; // small increase for same tribe but different village
}

void VillageControl::onMemberKilledOrStunned(Creature* victim, const Creature* killer) {
  if (isEnemy(killer))
    victims += 1;
}

template <typename TriggerType>
bool contains(const vector<AttackTrigger>& triggers) {
  for (auto& t : triggers)
    if (t.contains<TriggerType>())
      return true;
  return false;
}

bool VillageControl::canPillage(const Collective* by) const {
  PROFILE;
  auto getValue = [&] {
    for (auto& v : collective->getTerritory().getPillagePositions())
      if ((v.getCollective() == collective || !v.getCollective()) &&
          !by->getTerritory().contains(v) && !v.getItems().empty())
        return true;
    return false;
  };
  if (!canPillageCache)
    canPillageCache = getValue();
  return *canPillageCache;
}

void VillageControl::onEvent(const GameEvent& event) {
  using namespace EventInfo;
  event.visit<void>(
      [&](const ItemStolen& info) {
        canPillageCache = none;
        if (!collective->isConquered() && collective->getTerritory().contains(info.shopPosition)
            && behaviour && contains<StolenItems>(behaviour->triggers)
            && getEnemyCollective()
            && getEnemyCollective()->getCreatures().contains(info.creature)) {
          if (stolenItemCount == 0)
            info.creature->privateMessage(PlayerMessage("You are going to regret this", MessagePriority::HIGH));
          ++stolenItemCount;
        }
      },
      [&](const ItemsAppeared& info) {
        canPillageCache = none;
        if (collective->getTerritory().contains(info.position))
          for (auto& it : info.items)
            myItems.insert(it);
      },
      [&](const ItemsDropped& info) {
        canPillageCache = none;
      },
      [&](const ItemsPillaged& info) {
        canPillageCache = none;
      },
      [&](const ItemsPickedUp& info) {
        canPillageCache = none;
        int numStolen = 0;
        for (const Item* it : info.items)
          if (myItems.contains(it) && !it->getShopkeeper(info.creature))
            ++numStolen;
        if (numStolen > 0 && collective->getTerritory().contains(info.creature->getPosition()) &&
            !collective->isConquered() && (!getEnemyCollective()
              || getEnemyCollective()->getCreatures().contains(info.creature))) {
          info.creature->privateMessage(PlayerMessage("You are going to regret this", MessagePriority::HIGH));
          stolenItemCount += numStolen;
          collective->getTribe()->onItemsStolen(info.creature);
        }
      },
      [&](const FurnitureRemoved& info) {
        if (collective->getTerritory().contains(info.position) &&
            collective->getGame()->getContentFactory()->furniture.getData(info.type).isWall())
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

void VillageControl::updateAggression(EnemyAggressionLevel level) {
  switch (level) {
    case EnemyAggressionLevel::NONE:
      if (behaviour)
        behaviour->triggers.clear();
      break;
    case EnemyAggressionLevel::EXTREME:
      if (behaviour && !behaviour->triggers.empty())
        behaviour->triggers.push_back(Timer{1000});
      break;
    default:
      break;
  }
}

void VillageControl::launchAttack(vector<Creature*> attackers, bool duel) {
  auto enemy = getEnemyCollective();
  for (Creature* c : attackers) {
//      if (getCollective()->getGame()->canTransferCreature(c, enemy->getLevel()->getModel()))
    if (c->isAffected(LastingEffect::RIDER))
      if (auto steed = collective->getSteedOrRider(c))
        c->forceMount(steed);
    collective->getGame()->transferCreature(c, enemy->getModel());
  }
  optional<int> ransom;
  int hisGold = enemy->numResource(CollectiveResourceId("GOLD"));
  if (!duel && behaviour->ransom && hisGold >= behaviour->ransom->second)
    ransom = max<int>(behaviour->ransom->second,
        (Random.getDouble(behaviour->ransom->first * 0.6, behaviour->ransom->first)) * hisGold);
  TeamId team = collective->getTeams().create(attackers);
  if (behaviour->isAttackBehaviourNonChasing())
    collective->getTeams().setTeamOrder(team, TeamOrder::FLEE, true);
  collective->getTeams().activate(team);
  collective->freeTeamMembers(attackers);
  vector<const Task*> attackTasks;
  shared_ptr<DuelState> duelFlag;
  if (duel)
    duelFlag = make_shared<DuelState>(DuelState::INACTIVE);
  for (Creature* c : attackers) {
    auto task = Task::withTeam(collective, team, behaviour->getAttackTask(this));
    if (duelFlag)
      task = Task::duelTask(enemy, collective, attackers, std::move(task), duelFlag);
    attackTasks.push_back(task.get());
    collective->setPriorityTask(c, std::move(task));
  }
  enemy->getControl()->addAttack(CollectiveAttack(std::move(attackTasks), collective, attackers, ransom));
  attackSizes[team] = attackers.size();
}

bool VillageControl::considerVillainAmbush(const vector<Creature*>& travellers) {
  if (!behaviour)
    return false;
  if (Random.chance(behaviour->ambushChance)) {
    auto attackers = getAttackers();
    if (!attackers.empty()) {
      const int maxDist = 3;
      HashMap<Position, int> distance;
      queue<Position> q;
      auto add = [&](Position p, int dist) {
        q.push(p);
        distance[p] = dist;
      };
      for (auto c : travellers)
        add(c->getPosition(), 0);
      while (!q.empty()) {
        auto pos = q.front();
        q.pop();
        int dist = distance.at(pos);
        if (dist < maxDist)
          for (auto v : pos.neighbors8())
            if (!distance.count(v) && v.canEnterEmpty(attackers[0]))
              add(v, dist + 1);
      }
      auto goodPos = getKeys(distance).filter([&](auto pos) {
        if (distance.at(pos) != maxDist)
          return false;
        for (auto c : travellers)
          if (c->canSee(pos))
            return true;
        return false;
      });
      if (goodPos.size() < attackers.size())
        return false;
      goodPos = Random.permutation(std::move(goodPos));
      vector<Position> targets;
      for (Creature* c : attackers) {
        bool found = false;
        for (auto pos : goodPos)
          if (pos.canEnter(c)) {
            goodPos.removeElement(pos);
            targets.push_back(pos);
            found = true;
            break;
          }
        if (!found)
          return false;
      }
      CHECK(targets.size() == attackers.size());
      for (int i : All(attackers)) {
        auto c = attackers[i];
        if (c->isAffected(LastingEffect::RIDER))
          if (auto steed = collective->getSteedOrRider(c))
            c->forceMount(steed);
        Random.choose(travellers)->addCombatIntent(c, Creature::CombatIntentInfo::Type::CHASE);
        if (c->getPosition().getModel() == collective->getModel())
          c->getPosition().moveCreature(targets[i]);
        else
          collective->getModel()->transferCreature(c->getPosition().getModel()->extractCreature(c), {targets[i]});
      }
      return true;
    }
  }
  return false;
}

void VillageControl::launchAllianceAttack(vector<Collective*> allies) {
  if (Collective* enemy = getEnemyCollective()) {
    auto attackers = collective->getLeaders();
    for (auto c : collective->getCreatures(MinionTrait::FIGHTER))
      if (!attackers.contains(c))
        attackers.push_back(c);
    for (Creature* c : attackers) {
//      if (getCollective()->getGame()->canTransferCreature(c, enemy->getLevel()->getModel()))
        if (auto steed = collective->getSteedOrRider(c))
          c->forceMount(steed);
        collective->getGame()->transferCreature(c, enemy->getModel());
    }
    TeamId team = collective->getTeams().create(attackers);
    collective->getTeams().activate(team);
    collective->freeTeamMembers(attackers);
    vector<const Task*> attackTasks;
    for (Creature* c : attackers) {
      auto task = Task::allianceAttack(allies, enemy, getKillLeaderTask(enemy));
      attackTasks.push_back(task.get());
      collective->setTask(c, std::move(task));
    }
    attackSizes[team] = attackers.size();
  }
}

void VillageControl::considerCancellingAttack() {
  if (auto enemyCollective = getEnemyCollective())
    for (auto team : collective->getTeams().getAll())
      if (!cancelledAttacks.count(team)) {
        vector<Creature*> members = collective->getTeams().getMembers(team);
        if (members.size() < (attackSizes[team] + 1) / 2 || (members.size() == 1 &&
              members[0]->getBody().isSeriouslyWounded())) {
          for (Creature* c : members)
            collective->setTask(c, Task::chain(
                Task::transferTo(collective->getModel()),
                Task::goTo(Random.choose(collective->getTerritory().getAll()))
            ));
          collective->getTeams().setTeamOrder(team, TeamOrder::FLEE, true);
          if (auto& name = collective->getName())
            enemyCollective->addRecordedEvent("resisting the attack of " + name->full);
          cancelledAttacks.insert(team);
        }
      }
}

void VillageControl::onRansomPaid() {
  for (auto team : collective->getTeams().getAll()) {
    vector<Creature*> members = collective->getTeams().getMembers(team);
    for (Creature* c : members)
      collective->freeFromTask(c);
    collective->getTeams().cancel(team);
  }
}

vector<TriggerInfo> VillageControl::getAllTriggers(const Collective* against) const {
  if (isEnemy() && behaviour && against == getEnemyCollective())
    return behaviour->triggers.transform([&](auto& t) { return TriggerInfo{t, behaviour->getTriggerValue(t, this)}; });
  return {};
}

bool VillageControl::canPerformAttack() const {
  if (collective->isConquered())
    return false;
  // don't attack if player is not in the base site
  if (collective->getGame()->getCurrentModel() != collective->getGame()->getMainModel().get())
    return false;
  // don't attack during the day when having undead minions
  auto& sunlightInfo = collective->getGame()->getSunlightInfo();
  if (sunlightInfo.getState() != SunlightState::NIGHT || sunlightInfo.getTimeRemaining() < 300_visible)
    for (auto c : collective->getCreatures(MinionTrait::FIGHTER))
      if (c->isAffected(LastingEffect::SUNLIGHT_VULNERABLE))
        return false;
  return true;
}

void VillageControl::acceptImmigration() {
  for (int i : All(collective->getImmigration().getImmigrants()))
    collective->getImmigration().setAutoState(i, ImmigrantAutoState::AUTO_ACCEPT);
}

void VillageControl::healAllCreatures() {
  PROFILE;
  const double freq = 0.1;
  if (Random.chance(freq))
    for (auto c : collective->getCreatures())
      c->heal(0.002 / freq);
}

bool VillageControl::isEnemy() const {
  if (auto enemy = getEnemyCollective())
    return enemy->getTribe()->isEnemy(collective->getTribe());
  return false;
}

vector<Creature*> VillageControl::getAttackers() const {
  vector<Creature*> fighters = collective->getCreatures(MinionTrait::FIGHTER).filter([&](auto c) {
    return collective->getTeams().getContaining(c).empty() && !c->isAffected(LastingEffect::INSANITY)
        && !LastingEffects::restrictedMovement(c);
  });
  /*if (getCollective()->getGame()->isSingleModel())
    fighters = filter(fighters, [this] (const Creature* c) {
        return contains(getCollective()->getTerritory().getAll(), c->getPosition()); });*/
  /*if (auto& name = collective->getName())
    INFO << name->shortened << " fighters: " << int(fighters.size())
      << (!collective->getTeams().getAll().empty() ? " attacking " : "");*/
  vector<Creature*> allMembers = collective->getCreatures();
  if (fighters.size() >= behaviour->minTeamSize &&
      allMembers.size() >= behaviour->minPopulation + behaviour->minTeamSize)
    return Random.permutation(fighters).getPrefix(
          Random.get(behaviour->minTeamSize, min(fighters.size(), allMembers.size() - behaviour->minPopulation) + 1));
  return {};
}

void VillageControl::considerAcceptingPrisoners() {
  vector<Creature*> captured;
  for (auto pos : collective->getTerritory().getPillagePositions())
    if (auto c = pos.getCreature())
      if (collective->getTribe()->isEnemy(c)) {
        if (c->isAffected(LastingEffect::STUNNED))
          captured.push_back(c);
        else
          return;
      }
  for (auto c : captured)
    collective->takePrisoner(c);
}

void VillageControl::update(bool currentlyActive) {
  if (!currentlyActive)
    for (auto c : getCreatures())
      if (collective->getModel() == c->getPosition().getModel() &&
          !collective->getTerritory().contains(c->getPosition()))
        for (auto pos : Random.permutation(collective->getTerritory().getAll()))
          if (pos.canEnter(c)) {
            c->getPosition().moveCreature(pos);
            break;
          }
  considerAcceptingPrisoners();
  considerCancellingAttack();
  acceptImmigration();
  healAllCreatures();
  for (auto& c : collective->getCreatures(MinionTrait::FIGHTER))
    if (c->getBody().isHumanoid())
      if (!c->isAffected(BuffId("BRIDGE_BUILDING_SKILL")))
        c->addPermanentEffect(BuffId("BRIDGE_BUILDING_SKILL"), 1, false);
  for (auto team : collective->getTeams().getAll()) {
    for (const Creature* c : collective->getTeams().getMembers(team))
      if (!collective->hasTask(c)) {
        collective->getTeams().cancel(team);
        break;
      }
    return;
  }
  double updateFreq = 0.1;
  if (isEnemy() && canPerformAttack() && Random.chance(updateFreq) && behaviour) {
    auto enemy = getEnemyCollective();
    maxEnemyPower = max(maxEnemyPower, enemy->getDangerLevel());
    double prob = behaviour->getAttackProbability(this) / updateFreq;
    if (Random.chance(prob)) {
      bool duel = !collective->getLeaders().empty() && enemy->getLeaders().size() == 1 &&
          Random.chance(behaviour->duelChance);
      auto attackers = getAttackers();
      if (!attackers.empty()) {
        if (duel && !attackers.contains(collective->getLeaders()[0]))
          attackers.insert(0, collective->getLeaders()[0]);
        launchAttack(attackers, duel);
      }
    }
  }
}

