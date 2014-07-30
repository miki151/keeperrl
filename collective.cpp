#include "stdafx.h"
#include "collective.h"
#include "collective_control.h"
#include "creature.h"
#include "pantheon.h"
#include "effect.h"
#include "level.h"
#include "square.h"

template <class Archive>
void Collective::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(creatures)
    & SVAR(leader)
    & SVAR(taskMap)
    & SVAR(tribe)
    & SVAR(control)
    & SVAR(deityStanding)
    & SVAR(level);
  CHECK_SERIAL;
}

SERIALIZABLE(Collective);

Collective::Collective() : control(CollectiveControl::idle(this)) {
}

Collective::~Collective() {
}

void Collective::addCreature(Creature* c) {
  if (!tribe)
    tribe = c->getTribe();
  if (!level)
    level = c->getLevel();
  creatures.push_back(c);
}

vector<Creature*>& Collective::getCreatures() {
  return creatures;
}

const Creature* Collective::getLeader() const {
  return leader;
}

Creature* Collective::getLeader() {
  return leader;
}

Level* Collective::getLevel() {
  return NOTNULL(level);
}

const Level* Collective::getLevel() const {
  return NOTNULL(level);
}

void Collective::setLeader(Creature* c) {
  leader = c;
}

const Tribe* Collective::getTribe() const {
  return NOTNULL(tribe);
}

Tribe* Collective::getTribe() {
  return NOTNULL(tribe);
}

const vector<Creature*>& Collective::getCreatures() const {
  return creatures;
}

double Collective::getWarLevel() const {
  return control->getWarLevel();
}

MoveInfo Collective::getMove(Creature* c) {
  CHECK(control);
  CHECK(contains(creatures, c));
  if (Task* task = taskMap.getTask(c)) {
    if (task->isDone()) {
      taskMap.removeTask(task);
    } else
      return task->getMove(c);
  }
  PTask newTask = control->getNewTask(c);
  if (newTask)
    return taskMap.addTask(std::move(newTask), c)->getMove(c);
  else
    return control.get()->getMove(c);
}

void Collective::setControl(PCollectiveControl c) {
  control = std::move(c);
}

void Collective::considerHealingLeader() {
  if (Deity* deity = Deity::getDeity(EpithetId::HEALTH))
    if (getStanding(deity) > 0)
      if (Random.rollD(5 / getStanding(deity))) {
        if (leader->getHealth() < 1) {
          leader->you(MsgType::ARE, "healed by " + deity->getName());
          leader->heal(1, true);
        }
      }
}

void Collective::onAttackEvent(Creature* victim, Creature* attacker) {
  if (victim == leader) {
    if (Deity* deity = Deity::getDeity(EpithetId::LIGHTNING))
      if (getStanding(deity) > 0)
        if (Random.rollD(5 / getStanding(deity))) {
          attacker->you(MsgType::ARE, " struck by lightning!");
          attacker->bleed(Random.getDouble(0.2, 0.5));
        }
    if (Deity* deity = Deity::getDeity(EpithetId::DEFENSE))
      if (getStanding(deity) > 0)
        if (Random.rollD(10 / getStanding(deity))) {
          victim->globalMessage(
              deity->getName() + " sends " + deity->getGender().his() + " servant with help.");
          Effect::summon(leader, deity->getServant(), 1, 30);
        }
  }
}

void Collective::tick(double time) {
  control->tick(time);
  if (leader)
    considerHealingLeader();
}

vector<Creature*>& Collective::getCreatures(MinionTrait trait) {
  return byTrait[trait];
}

const vector<Creature*>& Collective::getCreatures(MinionTrait trait) const {
  return byTrait[trait];
}

void Collective::setTrait(Creature* c, MinionTrait trait) {
  byTrait[trait].push_back(c);
}
  
void Collective::onKillEvent(const Creature* victim, const Creature* killer) {
  if (contains(creatures, victim)) {
    control->onCreatureKilled(victim, killer);
    removeElement(creatures, victim);
    if (Task* task = taskMap.getTask(victim))
      taskMap.removeTask(task);
    for (MinionTrait t : ENUM_ALL(MinionTrait))
      if (contains(byTrait[t], victim))
        removeElement(byTrait[t], victim);
    for (Creature* c : creatures)
      c->addMorale(0.03);
  }
  if (contains(creatures, killer))
    for (Creature* c : creatures)
      c->addMorale(c == killer ? 0.25 : 0.03);
}

double Collective::getStanding(const Deity* d) const {
  if (deityStanding.count(d))
    return deityStanding.at(d);
  else
    return 0;
}

double Collective::getStanding(EpithetId id) const {
  if (Deity* d = Deity::getDeity(id))
    return getStanding(d);
  else
    return 0;
}

static double standingFun(double standing) {
  const double maxMult = 2.0;
  return pow(maxMult, standing);
}

double Collective::getTechCostMultiplier() const {
  return 1.0 / standingFun(getStanding(EpithetId::WISDOM));
}

double Collective::getCraftingMultiplier() const {
  return standingFun(getStanding(EpithetId::CRAFTS));
}

double Collective::getWarMultiplier() const {
  return standingFun(getStanding(EpithetId::WAR)) / standingFun(getStanding(EpithetId::LOVE));
}

double Collective::getBeastMultiplier() const {
  return 1.0 / standingFun(getStanding(EpithetId::NATURE));
}

double Collective::getUndeadMultiplier() const {
  return 1.0 / standingFun(getStanding(EpithetId::DEATH));
}

void Collective::onEpithetWorship(Creature* who, WorshipType type, EpithetId id) {
  if (type == WorshipType::DESTROY_ALTAR)
    return;
  double increase = 0;
  switch (type) {
    case WorshipType::PRAYER: increase = 1.0 / 400; break;
    case WorshipType::SACRIFICE: increase = 1.0 / 2; break;
    default: break;
  }
  switch (id) {
    case EpithetId::COURAGE: who->addMorale(increase); break;
    case EpithetId::FEAR: who->addMorale(-increase); break;
    default: break;
  }
}

void Collective::onWorshipEvent(Creature* who, const Deity* to, WorshipType type) {
  double increase = 0;
  switch (type) {
    case WorshipType::PRAYER: increase = 1.0 / 2000; break;
    case WorshipType::SACRIFICE: increase = 1.0 / 5; break;
    case WorshipType::DESTROY_ALTAR: deityStanding[to] = -1; return;
  }
  deityStanding[to] = min(1.0, deityStanding[to] + increase);
  for (EpithetId id : to->getEpithets())
    onEpithetWorship(who, type, id);
}

double Collective::getEfficiency(const Creature* c) const {
  return pow(2.0, c->getMorale());
}

