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
    & BOOST_SERIALIZATION_NVP(aliveCreatures)
    & BOOST_SERIALIZATION_NVP(villain)
    & BOOST_SERIALIZATION_NVP(level)
    & BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(tribe)
    & BOOST_SERIALIZATION_NVP(killedPoints)
    & BOOST_SERIALIZATION_NVP(killedCoeff)
    & BOOST_SERIALIZATION_NVP(powerCoeff)
    & BOOST_SERIALIZATION_NVP(lastAttack)
    & BOOST_SERIALIZATION_NVP(lastMyAttack)
    & BOOST_SERIALIZATION_NVP(lastAttackLaunched)
    & BOOST_SERIALIZATION_NVP(triggerAmounts)
    & BOOST_SERIALIZATION_NVP(fightingCreatures);
}

SERIALIZABLE(VillageControl);

VillageControl::VillageControl(Collective* c, const Level* l, string n, double kCoeff, double pCoeff) 
    : villain(c), level(l), name(n), killedCoeff(kCoeff), powerCoeff(pCoeff) {
}

VillageControl::~VillageControl() {
}

void VillageControl::addCreature(Creature* c) {
  if (tribe == nullptr) {
    tribe = c->getTribe();
    CHECK(tribe->getLeader());
  }
  CHECK(c->getTribe() == tribe);
  CHECK(triggerAmounts.empty()) << "Attacks have already been calculated";
  allCreatures.push_back(c);
}

// How long to wait between being attacked and attacking
const int attackDelay = 60;

// How long to wait between consecutive attacks from the same village
const int myAttacksDelay = 300;

double VillageControl::getCurrentTrigger() {
  double enemyPoints = killedCoeff * killedPoints + powerCoeff * villain->getDangerLevel();
  double currentTrigger = 0;
  for (double trigger : triggerAmounts)
    if (trigger <= enemyPoints)
      currentTrigger = trigger;
    else
      break;
  return currentTrigger;
}

void VillageControl::tick(double time) {
  if (triggerAmounts.empty())
    calculateAttacks();
  if (aliveCreatures.empty())
    return;
  if (lastAttack >= time - attackDelay || lastMyAttack >= time - myAttacksDelay)
    return;
  double lastAttackPoints = 0;
  for (const Creature* c : allCreatures)
    if (fightingCreatures.count(c))
        lastAttackPoints += c->getDifficultyPoints();
  bool firstAttack = lastAttackPoints == 0;
  double currentTrigger = getCurrentTrigger();
  if (lastAttackPoints < currentTrigger) {
    lastMyAttack = time;
    int numCreatures = 0;
    for (const Creature* c : allCreatures)
      if (!fightingCreatures.count(c) && !c->isDead()) {
        ++numCreatures;
        fightingCreatures.insert(c);
        if ((lastAttackPoints += c->getDifficultyPoints()) >= currentTrigger)
          break;
      }
    if (numCreatures > 0) {
      if (currentTrigger >= *triggerAmounts.rbegin() - 0.001)
        lastAttackLaunched = true;
      if (fightingCreatures.size() < allCreatures.size()) {
/*        if (numCreatures <= 3) {
          if (firstAttack)
            messageBuffer.addMessage(MessageBuffer::important("The " + tribe->getName() + 
                  " of " + name + " have sent a small group to scout your dungeon. "
                  "It it advisable that they never return from this mission."));
          else
            messageBuffer.addMessage(MessageBuffer::important("The " + tribe->getName() + 
                  " of " + name + " are sending another small group of scouts. How cute is that?"));
        } else*/
          messageBuffer.addMessage(MessageBuffer::important("The " + tribe->getName() + 
                " of " + name + " are attacking!"));
      } else
        messageBuffer.addMessage(MessageBuffer::important("The " + tribe->getName() + 
              " of " + name + " have undertaken a final, desperate attack!"));
    }
  }
}

bool VillageControl::isConquered() const {
  return aliveCreatures.empty();
}

void VillageControl::calculateAttacks() {
  CHECK(allCreatures[0] == tribe->getLeader());
  // Put the leader at the end
  std::swap(allCreatures[0], allCreatures.back());
  aliveCreatures = allCreatures;
  double myPower = 0;
  for (const Creature* c : allCreatures)
    myPower += c->getDifficultyPoints();
  Debug() << "Village power " << myPower;
  for (int i : Range(Random.getRandom(1, 3))) {
    double trigger = myPower * Random.getDouble(0.4, 1.2);
    triggerAmounts.insert(trigger);
    Debug() << "Village trigger " << trigger;
  }
}

bool VillageControl::startedAttack(Creature* creature) {
  return fightingCreatures.count(creature);
}

void VillageControl::onKillEvent(const Creature* victim, const Creature* killer) {
  if (victim->getTribe() == tribe)
    killedPoints += victim->getDifficultyPoints();
  if (contains(allCreatures, victim)) {
    lastAttack = victim->getTime();
    removeElement(aliveCreatures, victim);
    if (aliveCreatures.empty()) {
      messageBuffer.addMessage(MessageBuffer::important("You have exterminated the armed forces of " + name));
    } else {
      if (fightingCreatures.count(victim) && lastAttackLaunched && !NOTNULL(tribe->getLeader())->isDead()) {
        bool attackersDead = true;
        for (const Creature* c : fightingCreatures)
          if (!c->isDead())
            attackersDead = false;
        if (attackersDead)
          messageBuffer.addMessage(MessageBuffer::important("You have defeated the pathetic attacks of the "
                + tribe->getName() + " of " + name + ". Take advantage of their weakened defense and kill their "
                "leader, the " + tribe->getLeader()->getName()));
      }
    }
  }
}

class TopLevelVillageControl : public VillageControl {
  public:
  TopLevelVillageControl(Collective* villain, const Location* location, double killedCoeff, double powerCoeff) 
      : VillageControl(villain, location->getLevel(), location->getName(), killedCoeff, powerCoeff),
      villageLocation(location) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!startedAttack(c))
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

PVillageControl VillageControl::topLevelVillage(Collective* villain, const Location* location,
    double killedCoeff, double powerCoeff) {
  return PVillageControl(new TopLevelVillageControl(villain, location, killedCoeff, powerCoeff));
}


class DwarfVillageControl : public VillageControl {
  public:
  DwarfVillageControl(Collective* villain, const Level* level, StairDirection dir, StairKey key,
    double killedCoeff, double powerCoeff)
      : VillageControl(villain, level, "the Dwarven Halls", killedCoeff, powerCoeff), direction(dir), stairKey(key) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!startedAttack(c))
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
}

REGISTER_TYPES(VillageControl);

PVillageControl VillageControl::dwarfVillage(Collective* villain, const Level* level,
    StairDirection dir, StairKey key, double killedCoeff, double powerCoeff) {
  return PVillageControl(new DwarfVillageControl(villain, level, dir, key, killedCoeff, powerCoeff));
}

