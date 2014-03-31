#include "stdafx.h"

#include "village_control.h"
#include "message_buffer.h"
#include "collective.h"
#include "square.h"
#include "level.h"
#include "name_generator.h"

template <class Archive>
void VillageControl::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(EventListener)
    & SVAR(allCreatures)
    & SVAR(villain)
    & SVAR(level)
    & SVAR(name)
    & SVAR(tribe)
    & SVAR(attackTrigger)
    & SVAR(atWar);
  CHECK_SERIAL;
}

SERIALIZABLE(VillageControl);

template <class Archive>
void VillageControl::AttackTrigger::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(control)
    & SVAR(fightingCreatures);
  CHECK_SERIAL;
}

SERIALIZABLE(VillageControl::AttackTrigger);

VillageControl::VillageControl(Collective* c, const Level* l, string n, AttackTrigger* trigger) 
    : villain(c), level(l), name(n), attackTrigger(trigger) {
  attackTrigger->setVillageControl(this);
}

VillageControl::~VillageControl() {
}

void VillageControl::addCreature(Creature* c) {
  if (tribe == nullptr) {
    tribe = c->getTribe();
  }
  CHECK(c->getTribe() == tribe);
  allCreatures.push_back(c);
}

vector<const Creature*> VillageControl::getAliveCreatures() const {
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

bool VillageControl::AttackTrigger::startedAttack(const Creature* creature) {
  return fightingCreatures.count(creature);
}

void VillageControl::AttackTrigger::setVillageControl(VillageControl* c) {
  control = c;
}

void VillageControl::tick(double time) {
  attackTrigger->tick(time);
}

void VillageControl::onKillEvent(const Creature* victim, const Creature* killer) {
  if ((victim->getTribe() == tribe && (!killer ||  killer->getTribe() == Tribes::get(TribeId::KEEPER)))
      || (victim->getTribe() == Tribes::get(TribeId::KEEPER) && killer && killer->getTribe() == tribe))
    atWar = true;
  if (contains(allCreatures, victim)) {
    if (getAliveCreatures().empty()) {
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

class PowerTrigger : public VillageControl::AttackTrigger, public EventListener {
  public:
  // How long to wait between being attacked and attacking
  const int attackDelay = 100;

  // How long to wait between consecutive attacks from the same village
  const int myAttacksDelay = 300;

  PowerTrigger(double _killedCoeff, double _powerCoeff) : killedCoeff(_killedCoeff), powerCoeff(_powerCoeff) {}
  double getCurrentTrigger() {
    double enemyPoints = killedCoeff * killedPoints + powerCoeff * control->villain->getDangerLevel();
    Debug() << "Village " << control->name << " enemy points " << enemyPoints;
    double currentTrigger = 0;
    for (double trigger : triggerAmounts)
      if (trigger <= enemyPoints)
        currentTrigger = trigger;
      else
        break;
    return currentTrigger;
  }

  void calculateAttacks() {
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
    if (triggerAmounts.empty())
      calculateAttacks();
    if (control->getAliveCreatures().empty())
      return;
    if (lastAttack >= time - attackDelay || lastMyAttack >= time - myAttacksDelay)
      return;
    double lastAttackPoints = 0;
    for (const Creature* c : control->allCreatures)
      if (fightingCreatures.count(c))
        lastAttackPoints += c->getDifficultyPoints();
    bool firstAttack = lastAttackPoints == 0;
    double currentTrigger = getCurrentTrigger();
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
    ar& SUBCLASS(VillageControl::AttackTrigger)
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

VillageControl::AttackTrigger* VillageControl::getPowerTrigger(double killedCoeff, double powerCoeff) {
  return new PowerTrigger(killedCoeff, powerCoeff);
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

class FinalTrigger : public VillageControl::AttackTrigger {
  public:
  FinalTrigger(vector<VillageControl*> _controls) : controls(_controls) {}

  virtual void tick(double time) {
    if (totalWar)
      return;
    bool allConquered = true;
    for (VillageControl* c : controls)
      allConquered &= c->isConquered();
    if (allConquered && !control->isConquered()) {
      messageBuffer.addMessage(MessageBuffer::important(control->tribe->getLeader()->getTheName() + 
            " and his army have undertaken a final, desperate attack!"));
      totalWar = true;
      for (const Creature* c : control->allCreatures)
        fightingCreatures.insert(c);
    }
  }

  SERIALIZATION_CONSTRUCTOR(FinalTrigger);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(VillageControl::AttackTrigger)
      & SVAR(controls)
      & SVAR(totalWar);
    CHECK_SERIAL;
  }

  private:
  vector<VillageControl*> SERIAL(controls);
  bool SERIAL2(totalWar, false);
};

VillageControl::AttackTrigger* VillageControl::getFinalTrigger(vector<VillageControl*> otherControls) {
  return new FinalTrigger(otherControls);
}

class TopLevelVillageControl : public VillageControl {
  public:
  TopLevelVillageControl(Collective* villain, const Location* location, AttackTrigger* t) 
      : VillageControl(villain, location->getLevel(), location->getName(), t), villageLocation(location) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!attackTrigger->startedAttack(c))
      return NoMove;
    if (c->getLevel() != villain->getLevel())
      return NoMove;
    if (Optional<Vec2> move = c->getMoveTowards(villain->getKeeper()->getPosition())) 
      return {1.0, [this, move, c] () {
        c->move(*move);
      }};
    else {
      for (Vec2 v : Vec2::directions8(true))
        if (c->canDestroy(v) && c->getSquare(v)->getName() == "door")
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
      & SVAR(visited)
      & SVAR(villageLocation);
    CHECK_SERIAL;
  }

  unordered_set<Vec2> SERIAL(visited);
  const Location* SERIAL(villageLocation);
};

PVillageControl VillageControl::topLevelVillage(Collective* villain, const Location* location, AttackTrigger* t) {
  return PVillageControl(new TopLevelVillageControl(villain, location, t));
}


class DwarfVillageControl : public VillageControl {
  public:
  DwarfVillageControl(Collective* villain, const Level* level, StairDirection dir, StairKey key, AttackTrigger* t)
      : VillageControl(villain, level, "the Dwarven Halls", t), direction(dir), stairKey(key) {}

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
  REGISTER_TYPE(ar, FinalTrigger);
}

REGISTER_TYPES(VillageControl);

PVillageControl VillageControl::dwarfVillage(Collective* villain, const Level* level,
    StairDirection dir, StairKey key, AttackTrigger* t) {
  return PVillageControl(new DwarfVillageControl(villain, level, dir, key, t));
}

