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
    & BOOST_SERIALIZATION_NVP(allCreatures)
    & BOOST_SERIALIZATION_NVP(villain)
    & BOOST_SERIALIZATION_NVP(level)
    & BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(tribe)
    & BOOST_SERIALIZATION_NVP(attackTrigger);
}

SERIALIZABLE(VillageControl);

template <class Archive>
void VillageControl::AttackTrigger::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(control)
    & BOOST_SERIALIZATION_NVP(fightingCreatures);
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
  const int attackDelay = 60;

  // How long to wait between consecutive attacks from the same village
  const int myAttacksDelay = 300;

  PowerTrigger(double _killedCoeff, double _powerCoeff) : killedCoeff(_killedCoeff), powerCoeff(_powerCoeff) {}
  double getCurrentTrigger() {
    double enemyPoints = killedCoeff * killedPoints + powerCoeff * control->villain->getDangerLevel();
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
    Debug() << "Village power " << myPower;
    for (int i : Range(Random.getRandom(1, 3))) {
      double trigger = myPower * Random.getDouble(0.4, 1.2);
      triggerAmounts.insert(trigger);
      Debug() << "Village trigger " << trigger;
    }
  }

  void onKillEvent(const Creature* victim, const Creature* killer) override {
    if (victim->getTribe() == control->tribe)
      killedPoints += victim->getDifficultyPoints();
    if (contains(control->allCreatures, victim))
      lastAttack = victim->getTime();
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
      & BOOST_SERIALIZATION_NVP(killedPoints)
      & BOOST_SERIALIZATION_NVP(killedCoeff)
      & BOOST_SERIALIZATION_NVP(powerCoeff)
      & BOOST_SERIALIZATION_NVP(lastAttack)
      & BOOST_SERIALIZATION_NVP(lastMyAttack)
      & BOOST_SERIALIZATION_NVP(lastAttackLaunched)
      & BOOST_SERIALIZATION_NVP(triggerAmounts);
  }

  private:
  double killedPoints = 0;
  double killedCoeff;
  double powerCoeff;
  double lastAttack = 0;
  double lastMyAttack = 0;
  bool lastAttackLaunched = false;
  set<double> triggerAmounts;
};

VillageControl::AttackTrigger* VillageControl::getPowerTrigger(double killedCoeff, double powerCoeff) {
  return new PowerTrigger(killedCoeff, powerCoeff);
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
      & BOOST_SERIALIZATION_NVP(controls)
      & BOOST_SERIALIZATION_NVP(totalWar);
  }

  private:
  vector<VillageControl*> controls;
  bool totalWar = false;
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
      return NoMove;
    }
  }

  SERIALIZATION_CONSTRUCTOR(TopLevelVillageControl);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(VillageControl)
      & BOOST_SERIALIZATION_NVP(visited)
      & BOOST_SERIALIZATION_NVP(villageLocation);
  }

  unordered_set<Vec2> visited;
  const Location* villageLocation;
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
    ar& SUBCLASS(VillageControl) & BOOST_SERIALIZATION_NVP(direction) & BOOST_SERIALIZATION_NVP(stairKey);
  }

  StairDirection direction;
  StairKey stairKey;
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

