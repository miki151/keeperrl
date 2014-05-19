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
#include "message_buffer.h"
#include "collective.h"
#include "square.h"
#include "level.h"
#include "name_generator.h"
#include "model.h"

template <class Archive>
void VillageControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(EventListener) & SUBCLASS(Task::Callback)
    & SVAR(allCreatures)
    & SVAR(villain)
    & SVAR(level)
    & SVAR(name)
    & SVAR(tribe)
    & SVAR(attackTrigger)
    & SVAR(atWar)
    & SVAR(taskMap)
    & SVAR(beds);
  CHECK_SERIAL;
}

SERIALIZABLE(VillageControl);


VillageControl::VillageControl(Collective* c, const Location* loc)
    : villain(c), level(NOTNULL(loc->getLevel())), name(loc->hasName() ? loc->getName() : "") {
  for (Vec2 v : loc->getBounds())
    if (level->getSquare(v)->getApplyType(Creature::getDefault()) == SquareApplyType::SLEEP)
      beds.push_back(v);
}

VillageControl::VillageControl() {
}

VillageControl::~VillageControl() {
}

bool VillageControl::isAnonymous() const {
  return name.empty();
}

class AttackTrigger {
  public:
  AttackTrigger(VillageControl* c) : control(c) {
  }

  virtual void tick(double time) {};
  virtual bool startedAttack(const Creature*) = 0;
  virtual void init() {}

  virtual ~AttackTrigger() {}

  SERIALIZATION_CONSTRUCTOR(AttackTrigger);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SVAR(control);
    CHECK_SERIAL;
  }

  protected:
  VillageControl* SERIAL(control);
};

void VillageControl::initialize(vector<Creature*> c) {
  CHECK(!c.empty());
  tribe = c[0]->getTribe();
  allCreatures = c;
  attackTrigger->init();
}

vector<Creature*> VillageControl::getAliveCreatures() const {
  return filter(allCreatures, [](const Creature* c) { return !c->isDead(); });
}

bool VillageControl::currentlyAttacking() const {
  for (const Creature* c : getAliveCreatures())
    if (attackTrigger->startedAttack(c))
      return true;
  return false;
}

bool VillageControl::isConquered() const {
  return getAliveCreatures().empty();
}

static int expLevelFun(double time) {
  return max(0.0, time - 1000) / 500;
}

void VillageControl::tick(double time) {
  attackTrigger->tick(time);
  for (Creature* c : getAliveCreatures())
    if (c->getExpLevel() < expLevelFun(time))
      c->increaseExpLevel(1);
}

void VillageControl::onKillEvent(const Creature* victim, const Creature* killer) {
  if ((victim->getTribe() == tribe && (!killer ||  killer->getTribe() == Tribes::get(TribeId::KEEPER)))
      || (victim->getTribe() == Tribes::get(TribeId::KEEPER) && killer && killer->getTribe() == tribe))
    atWar = true;
  if (contains(allCreatures, victim)) {
    if (!isAnonymous() && getAliveCreatures().empty()) {
      messageBuffer.addMessage(MessageBuffer::important("You have exterminated the armed forces of " + name));
    } else {
 /*     if (fightingCreatures.count(victim) && lastAttackLaunched
          && tribe->getLeader() && tribe->getLeader()->isDead()) {
        bool attackersDead = true;
        for (const Creature* c : fightingCreatures)
          if (!c->isDead())
            attackersDead = false;
        if (attackersDead)
          messageBuffer.addMessage(MessageBuffer::important("You have defeated the pathetic attacks of the "
                + tribe->getName() + " of " + name + ". Take advantage of their weakened defense and kill their "
                "leader, the " + tribe->getLeader()->getName()));
      }*/
    }
  }
}

class AttackTriggerSet : public AttackTrigger {
  public:
  using AttackTrigger::AttackTrigger;

  virtual bool startedAttack(const Creature* c) override {
    return fightingCreatures.count(c);
  }

  SERIALIZATION_CONSTRUCTOR(AttackTriggerSet);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(AttackTrigger)
      & SVAR(fightingCreatures);
  }

  protected:
  set<const Creature*> SERIAL(fightingCreatures);
};

class PowerTrigger : public AttackTriggerSet, public EventListener {
  public:
  // How long to wait between being attacked and attacking
  const int attackDelay = 100;

  // How long to wait between consecutive attacks from the same village
  const int myAttacksDelay = 300;

  PowerTrigger(VillageControl* c, double _killedCoeff, double _powerCoeff)
      : AttackTriggerSet(c), killedCoeff(_killedCoeff), powerCoeff(_powerCoeff) {}

  double getCurrentTrigger(double time) {
    double enemyPoints = killedCoeff * killedPoints + powerCoeff * control->villain->getDangerLevel()
      + max(0.0, (time - 1000) / 10);
    Debug() << "Village " << control->name << " enemy points " << enemyPoints;
    double currentTrigger = 0;
    for (double trigger : triggerAmounts)
      if (trigger <= enemyPoints)
        currentTrigger = trigger;
      else
        break;
    return currentTrigger;
  }

  virtual void init() override {
    // If there is a leader then he is added first, so put him at the end
    std::swap(control->allCreatures[0], control->allCreatures.back());
    double myPower = 0;
    for (const Creature* c : control->allCreatures)
      myPower += c->getDifficultyPoints();
    Debug() << "Village " << control->name << " power " << myPower;
    for (int i : Range(Random.getRandom(1, 3))) {
      double trigger = myPower * Random.getDouble(0.4, 1.2);
      triggerAmounts.insert(trigger);
      Debug() << "Village " << control->name << " trigger " << trigger;
    }
  }

  void onKillEvent(const Creature* victim, const Creature* killer) override {
    if (victim->getTribe() == control->tribe && (!killer ||  killer->getTribe() == Tribes::get(TribeId::KEEPER))) {
      killedPoints += victim->getDifficultyPoints();
      lastAttack = victim->getTime();
    }
  }

  virtual void tick(double time) override {
    if (control->getAliveCreatures().empty())
      return;
    if (lastAttack >= time - attackDelay || lastMyAttack >= time - myAttacksDelay)
      return;
    double lastAttackPoints = 0;
    for (const Creature* c : control->allCreatures)
      if (fightingCreatures.count(c))
        lastAttackPoints += c->getDifficultyPoints();
    bool firstAttack = lastAttackPoints == 0;
    double currentTrigger = getCurrentTrigger(time);
    if (lastAttackPoints < currentTrigger) {
      lastMyAttack = time;
      int numCreatures = 0;
      for (const Creature* c : control->allCreatures)
        if (!fightingCreatures.count(c) && !c->isDead()) {
          ++numCreatures;
          fightingCreatures.insert(c);
          if ((lastAttackPoints += c->getDifficultyPoints()) >= currentTrigger)
            break;
        }
      if (numCreatures > 0) {
        if (currentTrigger >= *triggerAmounts.rbegin() - 0.001)
          lastAttackLaunched = true;
      //  if (fightingCreatures.size() < control->allCreatures.size()) {
          /*        if (numCreatures <= 3) {
                    if (firstAttack)
                    messageBuffer.addMessage(MessageBuffer::important("The " + tribe->getName() + 
                    " of " + name + " have sent a small group to scout your dungeon. "
                    "It it advisable that they never return from this mission."));
                    else
                    messageBuffer.addMessage(MessageBuffer::important("The " + tribe->getName() + 
                    " of " + name + " are sending another small group of scouts. How cute is that?"));
                    } else*/
        if (!control->isAnonymous())
          messageBuffer.addMessage(MessageBuffer::important("The " + control->tribe->getName() + 
                " of " + control->name + " are attacking!"));
//        } else
  //        messageBuffer.addMessage(MessageBuffer::important("The " + control->tribe->getName() + 
    //            " of " + control->name + " have undertaken a final, desperate attack!"));
      }
    }
  }

  SERIALIZATION_CONSTRUCTOR(PowerTrigger);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(AttackTriggerSet) & SUBCLASS(EventListener)
      & SVAR(killedPoints)
      & SVAR(killedCoeff)
      & SVAR(powerCoeff)
      & SVAR(lastAttack)
      & SVAR(lastMyAttack)
      & SVAR(lastAttackLaunched)
      & SVAR(triggerAmounts);
    CHECK_SERIAL;
  }

  private:
  double SERIAL2(killedPoints, 0);
  double SERIAL(killedCoeff);
  double SERIAL(powerCoeff);
  double SERIAL2(lastAttack, 0);
  double SERIAL2(lastMyAttack, 0);
  bool SERIAL2(lastAttackLaunched, false);
  set<double> SERIAL(triggerAmounts);
};

void VillageControl::setPowerTrigger(double killedCoeff, double powerCoeff) {
  attackTrigger.reset(new PowerTrigger(this, killedCoeff, powerCoeff));
}

class FirstContact : public AttackTrigger, public EventListener {
  public:
  FirstContact(VillageControl* c, PAttackTrigger o) : AttackTrigger(c), other(std::move(o)) {}

  virtual bool startedAttack(const Creature* c) {
    return madeContact && other->startedAttack(c);
  }

  virtual void tick(double time) override {
    if (madeContact)
      other->tick(time);
  }

  virtual void init() override {
    other->init();
  }

  virtual void onCombatEvent(const Creature* c) override {
    CHECK(c != nullptr);
    if (contains(control->allCreatures, c))
      madeContact = true;
  }

  SERIALIZATION_CONSTRUCTOR(FirstContact);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(AttackTrigger) & SUBCLASS(EventListener)
      & SVAR(other)
      & SVAR(madeContact);
    CHECK_SERIAL;
  }

  private:
  PAttackTrigger other;
  bool madeContact = false;
};

void VillageControl::setOnFirstContact() {
  attackTrigger.reset(new FirstContact(this, std::move(attackTrigger)));
}

View::GameInfo::VillageInfo::Village VillageControl::getVillageInfo() const {
  View::GameInfo::VillageInfo::Village info;
  info.name = name;
  info.tribeName = tribe->getName();
  if (currentlyAttacking())
    info.state = "attacking!";
  else if (isConquered())
    info.state = "conquered";
  else if (atWar)
    info.state = "war";
  return info;
}

class FinalTrigger : public AttackTriggerSet {
  public:
  FinalTrigger(VillageControl* c, vector<VillageControl*> _controls) : AttackTriggerSet(c), controls(_controls) {}

  virtual void tick(double time) {
    if (totalWar < 0)
      return;
    bool allConquered = true;
    for (VillageControl* c : controls)
      allConquered &= c->isConquered();
    if (allConquered && !control->isConquered() && totalWar == 100000)
      totalWar = time + Random.getRandom(80, 200);
    if (totalWar < time) {
      messageBuffer.addMessage(MessageBuffer::important(control->tribe->getLeader()->getTheName() + 
            " and his army have undertaken a final, desperate attack!"));
      totalWar = -1;
      for (const Creature* c : control->allCreatures)
        fightingCreatures.insert(c);
    }
  }

  SERIALIZATION_CONSTRUCTOR(FinalTrigger);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(AttackTriggerSet)
      & SVAR(controls)
      & SVAR(totalWar);
    CHECK_SERIAL;
  }

  private:
  vector<VillageControl*> SERIAL(controls);
  double SERIAL2(totalWar, 100000);
};

void VillageControl::setFinalTrigger(vector<VillageControl*> otherControls) {
  attackTrigger.reset(new FinalTrigger(this, otherControls));
}

class TopLevelVillageControl : public VillageControl {
  public:
  TopLevelVillageControl(Collective* villain, const Location* location) : VillageControl(villain, location) {}

  MoveInfo getPeacefulMove(Creature* c) {
    if (Task* task = taskMap.getTask(c)) {
      if (task->isDone()) {
        taskMap.removeTask(task);
      } else
        return task->getMove(c);
    }
    if (c->getLevel()->getModel()->getSunlightInfo().state == Model::SunlightInfo::NIGHT 
        && !c->isAffected(Creature::SLEEP) && c->isHumanoid()) {
      for (Vec2 v : beds)
        if (level->getSquare(v)->canEnter(c))
          return taskMap.addTask(Task::applySquare(this, {v}), c)->getMove(c);
    }
    return NoMove;
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!attackTrigger->startedAttack(c))
      return getPeacefulMove(c);
    if (c->getLevel() != villain->getLevel())
      return NoMove;
    if (Optional<Vec2> move = c->getMoveTowards(villain->getKeeper()->getPosition())) 
      return {1.0, [this, move, c] () {
        c->move(*move);
      }};
    else {
      for (Vec2 v : Vec2::directions8(true))
        if (c->canDestroy(v))
          return {1.0, [this, v, c] () {
            c->destroy(v);
          }};
      return {1.0, [c] () {
        c->wait();
      }};
    }
  }

  SERIALIZATION_CONSTRUCTOR(TopLevelVillageControl);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(VillageControl)
      & SVAR(visited);
    CHECK_SERIAL;
  }

  unordered_set<Vec2> SERIAL(visited);
};

PVillageControl VillageControl::topLevelVillage(Collective* villain, const Location* location) {
  return PVillageControl(new TopLevelVillageControl(villain, location));
}

PVillageControl VillageControl::topLevelAnonymous(Collective* villain, const Location* location) {
  return PVillageControl(new TopLevelVillageControl(villain, location));
}


class DwarfVillageControl : public VillageControl {
  public:
  DwarfVillageControl(Collective* villain, Level* level, StairDirection dir, StairKey key)
      : VillageControl(villain, new Location(level, level->getBounds())), direction(dir), stairKey(key) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!attackTrigger->startedAttack(c))
      return NoMove;
    if (c->getLevel() == villain->getLevel()) {
      if (Optional<Vec2> move = c->getMoveTowards(villain->getKeeper()->getPosition())) 
        return {1.0, [this, move, c] () {
          c->move(*move);
        }};
      else {
        for (Vec2 v : Vec2::directions8(true))
          if (c->canDestroy(v))
            return {1.0, [this, v, c] () {
              c->destroy(v);
            }};
        return NoMove;
      }
    } else {
      Vec2 stairs = getOnlyElement(c->getLevel()->getLandingSquares(direction, stairKey));
      if (c->getPosition() == stairs)
        return {1.0, [this, c] () {
          c->applySquare();
        }};
      if (Optional<Vec2> move = c->getMoveTowards(stairs)) 
          return {1.0, [=] () {
            c->move(*move);
          }};
        else
          return NoMove;
    }
  }

  SERIALIZATION_CONSTRUCTOR(DwarfVillageControl);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(VillageControl) & SVAR(direction) & SVAR(stairKey);
    CHECK_SERIAL;
  }

  StairDirection SERIAL(direction);
  StairKey SERIAL(stairKey);
};

template <class Archive>
void VillageControl::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, TopLevelVillageControl);
  REGISTER_TYPE(ar, DwarfVillageControl);
  REGISTER_TYPE(ar, PowerTrigger);
  REGISTER_TYPE(ar, AttackTriggerSet);
  REGISTER_TYPE(ar, FirstContact);
  REGISTER_TYPE(ar, FinalTrigger);
}

REGISTER_TYPES(VillageControl);

PVillageControl VillageControl::dwarfVillage(Collective* villain, Level* level,
    StairDirection dir, StairKey key) {
  return PVillageControl(new DwarfVillageControl(villain, level, dir, key));
}

