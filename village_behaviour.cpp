#include "stdafx.h"
#include "village_behaviour.h"
#include "creature.h"
#include "task.h"
#include "village_control.h"
#include "collective.h"
#include "game.h"
#include "collective_name.h"
#include "attack_trigger.h"
#include "creature_factory.h"
#include "construction_map.h"

SERIALIZE_DEF(VillageBehaviour, minPopulation, minTeamSize, triggers, attackBehaviour, welcomeMessage, ransom);

VillageBehaviour::VillageBehaviour() {}

VillageBehaviour::~VillageBehaviour() {}

PTask VillageBehaviour::getAttackTask(VillageControl* self) {
  Collective* enemy = self->getEnemyCollective();
  switch (attackBehaviour->getId()) {
    case AttackBehaviourId::KILL_LEADER:
      return Task::attackLeader(enemy);
    case AttackBehaviourId::KILL_MEMBERS:
      return Task::killFighters(enemy, attackBehaviour->get<int>());
    case AttackBehaviourId::STEAL_GOLD:
      return Task::stealFrom(enemy, self->getCollective());
    case AttackBehaviourId::CAMP_AND_SPAWN:
      return Task::campAndSpawn(enemy,
            attackBehaviour->get<CreatureFactory>(), Random.get(3, 7), Range(3, 7), Random.get(3, 7));
  }
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
  else if (victims <= 3)
    return 0.3;
  else if (victims <= 5)
    return 0.7;
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

static double getRoomProb(FurnitureType id) {
  switch (id) {
    case FurnitureType::THRONE: return 0.001;
    case FurnitureType::IMPALED_HEAD: return 0.000125;
    default: FATAL << "Unsupported ROOM_BUILT type"; return 0;
  }
}

static double getFinishOffProb(double maxPower, double currentPower, double selfPower) {
  if (maxPower < selfPower || currentPower * 2 >= maxPower)
    return 0;
  double minProb = 0.25;
  return 1 - 2 * (currentPower / maxPower) * (1 - minProb);
}

double VillageBehaviour::getTriggerValue(const Trigger& trigger, const VillageControl* self) const {
  double powerMaxProb = 1.0 / 10000; // rather small chance that they attack just because you are strong
  double victimsMaxProb = 1.0 / 500;
  double populationMaxProb = 1.0 / 500;
  double goldMaxProb = 1.0 / 1000;
  double stolenMaxProb = 1.0 / 300;
  double entryMaxProb = 1.0 / 20.0;
  double finishOffMaxProb = 1.0 / 1000;
  double proximityMaxProb = 1.0 / 5000;
  double timerProb = 1.0 / 3000;
  if (Collective* collective = self->getEnemyCollective())
    switch (trigger.getId()) {
      case AttackTriggerId::TIMER: 
        return collective->getGlobalTime() >= trigger.get<int>() ? timerProb : 0;
      case AttackTriggerId::ROOM_BUILT: 
        return collective->getConstructions().getBuiltCount(trigger.get<FurnitureType>()) *
          getRoomProb(trigger.get<FurnitureType>());
      case AttackTriggerId::POWER: 
        return powerMaxProb *
            powerClosenessFun(self->getCollective()->getDangerLevel(), collective->getDangerLevel());
      case AttackTriggerId::FINISH_OFF:
        return finishOffMaxProb * getFinishOffProb(self->maxEnemyPower, collective->getDangerLevel(),
            self->getCollective()->getDangerLevel());
      case AttackTriggerId::SELF_VICTIMS:
        return victimsMaxProb * victimsFun(self->victims, 0);
      case AttackTriggerId::ENEMY_POPULATION:
        return populationMaxProb * populationFun(
            collective->getCreatures(MinionTrait::FIGHTER).size(), trigger.get<int>());
      case AttackTriggerId::GOLD:
        return goldMaxProb * goldFun(collective->numResource(Collective::ResourceId::GOLD), trigger.get<int>());
      case AttackTriggerId::STOLEN_ITEMS:
        return stolenMaxProb 
          * stolenItemsFun(self->stolenItemCount);
      case AttackTriggerId::ENTRY:
        return entryMaxProb * self->entries;
      case AttackTriggerId::PROXIMITY:
        if (!collective->getGame()->isSingleModel() &&
            collective->getGame()->getModelDistance(collective, self->getCollective()) <= 1)
          return proximityMaxProb;
        else
          return 0;
    }
  return 0;
}

double VillageBehaviour::getAttackProbability(const VillageControl* self) const {
  double ret = 0;
  for (auto& elem : triggers) {
    double val = getTriggerValue(elem, self);
    CHECK(val >= 0 && val <= 1);
    ret = max(ret, val);
    INFO << "trigger " << EnumInfo<AttackTriggerId>::getString(elem.getId()) << " village "
        << self->getCollective()->getName().getFull() << " under attack probability " << val;
  }
  return ret;
}

