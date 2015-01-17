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

#include "monster_ai.h"
#include "square.h"
#include "level.h"
#include "collective.h"
#include "effect.h"
#include "item.h"
#include "creature.h"

class Behaviour {
  public:
  Behaviour(Creature*);
  virtual MoveInfo getMove() { return NoMove; }
  virtual void onAttacked(const Creature* attacker) {}
  virtual double itemValue(const Item*) { return 0; }
  Item* getBestWeapon();
  const Creature* getClosestEnemy();
  MoveInfo tryEffect(EffectType, double maxTurns);
  MoveInfo tryEffect(DirEffectType, Vec2);

  virtual ~Behaviour() {}

  SERIALIZATION_DECL(Behaviour);

  protected:
  Creature* SERIAL(creature);
};

MonsterAI::~MonsterAI() {}

template <class Archive> 
void MonsterAI::serialize(Archive& ar, const unsigned int version) {
   ar & SVAR(behaviours)
    & SVAR(weights)
    & SVAR(creature)
    & SVAR(pickItems);
  CHECK_SERIAL;
}

SERIALIZABLE(MonsterAI);

SERIALIZATION_CONSTRUCTOR_IMPL(MonsterAI);

template <class Archive> 
void Behaviour::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(creature);
  CHECK_SERIAL;
}


Behaviour::Behaviour(Creature* c) : creature(c) {
}

SERIALIZATION_CONSTRUCTOR_IMPL(Behaviour);

const Creature* Behaviour::getClosestEnemy() {
  int dist = 1000000000;
  const Creature* result = nullptr;
  for (const Creature* other : creature->getVisibleEnemies()) {
    if ((other->getPosition() - creature->getPosition()).length8() < dist
        && other->getTribe() != Tribe::get(TribeId::PEST)) {
      result = other;
      dist = (other->getPosition() - creature->getPosition()).length8();
    }
  }
  return result;
}

Item* Behaviour::getBestWeapon() {
  Item* best = nullptr;
  int damage = -1;
  for (Item* item : creature->getEquipment().getItems(Item::classPredicate(ItemClass::WEAPON))) 
    if (item->getModifier(ModifierType::DAMAGE) > damage) {
      damage = item->getModifier(ModifierType::DAMAGE);
      best = item;
    }
  return best;
}

MoveInfo Behaviour::tryEffect(EffectType type, double maxTurns) {
  for (Spell* spell : creature->getSpells()) {
    if (spell->hasEffect(type))
      if (auto action = creature->castSpell(spell))
        return { 1, action };
  }
  auto items = creature->getEquipment().getItems(Item::effectPredicate(type)); 
  for (Item* item : items)
    if (item->getApplyTime() <= maxTurns)
      if (auto action = creature->applyItem(item))
        return { 1, action};
  return NoMove;
}

MoveInfo Behaviour::tryEffect(DirEffectType type, Vec2 dir) {
  for (Spell* spell : creature->getSpells()) {
    if (spell->hasEffect(type))
      if (auto action = creature->castSpell(spell, dir))
        return { 1, action };
  }
  return NoMove;
}

class Heal : public Behaviour {
  public:
  Heal(Creature* c, bool _useBeds = true) : Behaviour(c), useBeds(_useBeds) {}

  virtual double itemValue(const Item* item) {
    if (item->getEffectType() == EffectType(EffectId::HEAL)) {
      return 0.5;
    }
    else
      return 0;
  }

  virtual MoveInfo getMove() {
    int healingRadius = 2;
    for (Square* square : creature->getSquares(
          Rectangle(-healingRadius, -healingRadius, healingRadius, healingRadius).getAllSquares()))
      if (const Creature* other = square->getCreature())
        if (creature->isFriend(other))
          if (auto action = creature->heal(other->getPosition() - creature->getPosition()))
            return {0.5, action};
    if (!creature->isHumanoid())
      return NoMove;
    if (creature->isAffected(LastingEffect::POISON)) {
      if (MoveInfo move = tryEffect(EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT), 1))
        return move;
      if (MoveInfo move = tryEffect(EffectType(EffectId::CURE_POISON), 1))
        return move;
    }
    if (creature->getHealth() == 1)
      return NoMove;
    if (MoveInfo move = tryEffect(EffectId::HEAL, 1))
      return move.setValue(min(1.0, 1.5 - creature->getHealth()));
    if (MoveInfo move = tryEffect(EffectId::HEAL, 3))
      return move.setValue(0.5 * min(1.0, 1.5 - creature->getHealth()));
    if (creature->getSquare()->getApplyType(creature) == SquareApplyType::SLEEP)
      return { 0.4 * min(1.0, 1.5 - creature->getHealth()), creature->applySquare()};
    Vec2 bedRadius(10, 10);
    if (useBeds && creature->canSleep() && !creature->isAffected(LastingEffect::POISON)) {
      if (!hasBed || hasBed->level != creature->getLevel()) {
        for (Square* square : creature->getSquares(Rectangle(-bedRadius, bedRadius).getAllSquares()))
          if (square->getApplyType(creature) == SquareApplyType::SLEEP)
            if (auto action = creature->moveTowards(square->getPosition())) {
              hasBed = BedInfo{square->getPosition(), creature->getLevel() };
              return { 0.4 * min(1.0, 1.5 - creature->getHealth()), action};
            }
      } else 
        if (auto action = creature->moveTowards(hasBed->pos))
          return { 0.4 * min(1.0, 1.5 - creature->getHealth()), action};
    }

    return NoMove;
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(hasBed) & SVAR(useBeds);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Heal);

  private:
  struct BedInfo {
    Vec2 pos;
    const Level* level;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar & BOOST_SERIALIZATION_NVP(pos) & BOOST_SERIALIZATION_NVP(level);
    }
  };
  optional<BedInfo> SERIAL(hasBed);
  bool SERIAL(useBeds);
};

class Rest : public Behaviour {
  public:
  Rest(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() {
    return {0.1, creature->wait() };
  }

  SERIALIZATION_CONSTRUCTOR(Rest);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour);
  }
};

class MoveRandomly : public Behaviour {
  public:
  MoveRandomly(Creature* c, int _memSize) 
      : Behaviour(c), memSize(_memSize) {}

  virtual MoveInfo getMove() {
    if (!visited(creature->getPosition()))
      updateMem(creature->getPosition());
    Vec2 direction(0, 0);
    double val = 0.0001;
    if (Random.roll(2))
      return {val, creature->wait()};
    Vec2 pos = creature->getPosition();
    for (Vec2 dir : Vec2::directions8(true))
      if (!visited(pos + dir) && creature->move(dir)) {
        direction = dir;
        break;
      }
    if (direction == Vec2(0, 0))
      for (Vec2 dir : Vec2::directions8(true))
        if (creature->move(dir)) {
          direction = dir;
          break;
        }
    if (direction == Vec2(0, 0))
      return {val, creature->wait() };
    else
      return {val, creature->move(direction).append([=] {
          updateMem(creature->getPosition());
      })};
  }

  void updateMem(Vec2 pos) {
    memory.push_back(pos);
    if (memory.size() > memSize)
      memory.pop_front();
  }

  bool visited(Vec2 pos) {
    return contains(memory, pos);
  }

  SERIALIZATION_CONSTRUCTOR(MoveRandomly);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(memory) & SVAR(memSize);
    CHECK_SERIAL;
  }

  private:
  deque<Vec2> SERIAL(memory);
  int SERIAL(memSize);
};

class AttackPest : public Behaviour {
  public:
  AttackPest(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    if (creature->getTribe() == Tribe::get(TribeId::PEST))
      return NoMove;
    const Creature* other = nullptr;
    for (Square* square : creature->getSquares(Vec2::directions8(true)))
      if (const Creature* c = square->getCreature())
        if (c->getTribe() == Tribe::get(TribeId::PEST)) {
          other = c;
          break;
        }
    if (!other)
      return NoMove;
    if (CreatureAction action = creature->attack(other))
      return {1.0, action};
    else
      return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(AttackPest);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour);
  }
};

class BirdFlyAway : public Behaviour {
  public:
  BirdFlyAway(Creature* c, double _maxDist) : Behaviour(c), maxDist(_maxDist) {}

  virtual MoveInfo getMove() override {
    const Creature* enemy = getClosestEnemy();
    if (Random.roll(15) || ( enemy && (enemy->getPosition() - creature->getPosition()).lengthD() < maxDist))
      if (auto action = creature->flyAway())
        return {1.0, action};
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(BirdFlyAway);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(maxDist);
    CHECK_SERIAL;
  }

  private:
  double SERIAL(maxDist);
};

class GoldLust : public Behaviour {
  public:
  GoldLust(Creature* c) : Behaviour(c) {}

  virtual double itemValue(const Item* item) {
    if (item->getClass() == ItemClass::GOLD)
      return 1;
    else
      return 0;
  }

  SERIALIZATION_CONSTRUCTOR(GoldLust);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour);
  }
};

class Fighter : public Behaviour {
  public:
  Fighter(Creature* c, double powerR, bool _chase) : Behaviour(c), maxPowerRatio(powerR), chase(_chase) {
  }

  virtual ~Fighter() {
  }

  REGISTER_HANDLER(KillEvent, const Creature* victim, const Creature* killer) {
    if (lastSeen && victim == lastSeen->creature)
      lastSeen = none;
  }

  REGISTER_HANDLER(ThrowEvent, const Level* l, const Creature* thrower, const Item* item, const vector<Vec2>& traj) {
    if (!creature->isHumanoid() || l != creature->getLevel())
      return;
    string name = creature->getName().bare();
    if (contains(traj, creature->getPosition())
          && item->getName().size() > name.size() && item->getName().substr(0, name.size()) == name) {
      creature->globalMessage(creature->getName().the() + " screams in terror!", "You hear a scream of terror.");
      if (Random.roll(2))
        creature->addEffect(LastingEffect::RAGE, (30));
      else
        creature->addMorale(-0.5);
    }
  }

  double getMoraleBonus() {
    return creature->getCourage() * pow(2.0, creature->getMorale());
  }

  virtual MoveInfo getMove() override {
    const Creature* other = getClosestEnemy();
    if (other != nullptr) {
      double myDamage = creature->getModifier(ModifierType::DAMAGE);
      Item* weapon = getBestWeapon();
      if (!creature->getWeapon() && weapon)
        myDamage += weapon->getModifier(ModifierType::DAMAGE);
      double powerRatio = getMoraleBonus() * myDamage / other->getModifier(ModifierType::DAMAGE);
      bool significantEnemy = myDamage < 5 * other->getModifier(ModifierType::DAMAGE);
      double weight = 1. - creature->getHealth() * 0.9;
      if (powerRatio < maxPowerRatio)
        weight += 2 - powerRatio * 2;
      weight = min(1.0, max(0.0, weight));
      if (creature->isAffected(LastingEffect::PANIC))
        weight = 1;
      if (other->isAffected(LastingEffect::SLEEP) || other->isStationary())
        weight = 0;
      Debug() << creature->getName().bare() << " panic weight " << weight;
      if (weight >= 0.5) {
        double dist = creature->getPosition().dist8(other->getPosition());
        if (dist < 7) {
          if (dist == 1 && creature->isHumanoid())
            GlobalEvents.addSurrenderEvent(creature, other);
          if (MoveInfo move = getPanicMove(other, weight))
            return move;
          else
            return getAttackMove(other, significantEnemy && chase);
        }
        return NoMove;
      } else
        return getAttackMove(other, significantEnemy && chase);
    } else
      return getLastSeenMove();
  }

  MoveInfo getPanicMove(const Creature* other, double weight) {
    if (auto teleMove = tryEffect(EffectId::TELEPORT, 1))
      return {weight, teleMove.move};
    if (other->getPosition().dist8(creature->getPosition()) > 3)
      if (auto move = getFireMove(other->getPosition() - creature->getPosition()))
        return {weight, move.move};
    if (auto action = creature->moveAway(other->getPosition(), chase))
      return {weight, action.prepend([=] {
        GlobalEvents.addCombatEvent(creature);
        GlobalEvents.addCombatEvent(other);
        lastSeen = {creature->getPosition(), creature->getTime(), creature->getLevel(), LastSeen::PANIC, other};
      })};
    else
      return NoMove;
  }

  virtual double itemValue(const Item* item) {
    if (contains<EffectType>({
          EffectType(EffectId::LASTING, LastingEffect::INVISIBLE),
          EffectType(EffectId::LASTING, LastingEffect::SLOWED),
          EffectType(EffectId::LASTING, LastingEffect::BLIND),
          EffectType(EffectId::LASTING, LastingEffect::SLEEP),
          EffectType(EffectId::LASTING, LastingEffect::POISON),
          EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT),
          EffectId::CURE_POISON,
          EffectId::TELEPORT,
          EffectType(EffectId::LASTING, LastingEffect::STR_BONUS),
          EffectType(EffectId::LASTING, LastingEffect::DEX_BONUS)},
          item->getEffectType()))
      return 1;
    if (item->getClass() == ItemClass::AMMO && creature->getSkillValue(Skill::get(SkillId::ARCHERY)) > 0)
      return 0.1;
    if (item->getClass() != ItemClass::WEAPON || creature->getAttr(AttrType::STRENGTH) < item->getMinStrength())
      return 0;
    if (item->getModifier(ModifierType::THROWN_DAMAGE) > 0)
      return (double)item->getModifier(ModifierType::THROWN_DAMAGE) / 50;
    int damage = item->getModifier(ModifierType::DAMAGE);
    Item* best = getBestWeapon();
    if (best && best != item && best->getModifier(ModifierType::DAMAGE) >= damage)
        return 0;
    return (double)damage / 50;
  }

  bool checkFriendlyFire(Vec2 enemyDir) {
    Vec2 dir = enemyDir.shorten();
    for (Vec2 v = dir; v != enemyDir; v += dir) {
      const Creature* c = creature->getSafeSquare(v)->getCreature();
      if (c && !creature->isEnemy(c))
        return true;
    }
    return false;
  }

  double getThrowValue(Item* it) {
    if (contains<EffectType>({
          EffectType(EffectId::LASTING, LastingEffect::POISON),
          EffectType(EffectId::LASTING, LastingEffect::SLOWED),
          EffectType(EffectId::LASTING, LastingEffect::BLIND),
          EffectType(EffectId::LASTING, LastingEffect::SLEEP)},
          it->getEffectType()))
      return 100;
    return it->getModifier(ModifierType::THROWN_DAMAGE);
  }

  MoveInfo getThrowMove(Vec2 enemyDir) {
    if (enemyDir.x != 0 && enemyDir.y != 0 && abs(enemyDir.x) != abs(enemyDir.y))
      return NoMove;
    if (checkFriendlyFire(enemyDir))
      return NoMove;
    Vec2 dir = enemyDir.shorten();
    Item* best = nullptr;
    int damage = 0;
    auto items = creature->getEquipment().getItems([this](Item* item) {
        return !creature->getEquipment().isEquiped(item);});
    for (Item* item : items)
      if (getThrowValue(item) > damage) {
        damage = getThrowValue(item);
        best = item;
      }
    if (best)
      if (auto action = creature->throwItem(best, dir)) {
        GlobalEvents.addCombatEvent(creature);
        return {1.0, action };
      }
    return NoMove;
  }

  vector<DirEffectType> getOffensiveEffects() {
    return makeVec<DirEffectType>(
        DirEffectId::BLAST,
        DirEffectType(DirEffectId::CREATURE_EFFECT, EffectType(EffectId::LASTING, LastingEffect::STUNNED))
    );
  }

  MoveInfo getFireMove(Vec2 dir) {
    if (dir.x != 0 && dir.y != 0 && abs(dir.x) != abs(dir.y))
      return NoMove;
    if (checkFriendlyFire(dir))
      return NoMove;
    for (auto effect : getOffensiveEffects())
      if (auto action = tryEffect(effect, dir.shorten()))
        return action;
    if (auto action = creature->fire(dir.shorten()))
      return {1.0, action.append([=] {
          GlobalEvents.addCombatEvent(creature);
      })};
    return NoMove;
  }

  MoveInfo getLastSeenMove() {
    if (!lastSeen) {
      return NoMove;
    }
    double lastSeenTimeout = 20;
    if (lastSeen->level != creature->getLevel() ||
        lastSeen->time < creature->getTime() - lastSeenTimeout ||
        lastSeen->pos == creature->getPosition()) {
      lastSeen = none;
      return NoMove;
    }
    if (chase && lastSeen->type == LastSeen::ATTACK)
      if (auto action = creature->moveTowards(lastSeen->pos)) {
        return {0.5, action.append([=] {
            GlobalEvents.addCombatEvent(creature);
        })};
      }
    if (lastSeen->type == LastSeen::PANIC && lastSeen->pos.dist8(creature->getPosition()) < 4)
      if (auto action = creature->moveAway(lastSeen->pos, chase))
        return {0.5, action.append([=] {
            GlobalEvents.addCombatEvent(creature);
        })};
    return NoMove;
  }

  MoveInfo getAttackMove(const Creature* other, bool chase) {
    int radius = 4;
    int distance = 10000;
    CHECK(other);
    if (other->isInvincible())
      return NoMove;
    Debug() << creature->getName().bare() << " enemy " << other->getName().bare();
    Vec2 enemyDir = (other->getPosition() - creature->getPosition());
    distance = enemyDir.length8();
    if (creature->isHumanoid() && !creature->getWeapon()) {
      if (Item* weapon = getBestWeapon())
        if (auto action = creature->equip(weapon))
          return {3.0 / (2.0 + distance), action.prepend([=] {
            GlobalEvents.addCombatEvent(creature);
            GlobalEvents.addCombatEvent(other);
            creature->globalMessage(creature->getName().the() + " draws " + weapon->getAName());
        })};
    }
    if (distance <= 5)
      for (EffectType effect : {
          EffectType(EffectId::LASTING, LastingEffect::INVISIBLE),
          EffectType(EffectId::LASTING, LastingEffect::STR_BONUS),
          EffectType(EffectId::LASTING, LastingEffect::DEX_BONUS),
          EffectType(EffectId::LASTING, LastingEffect::SPEED),
          EffectType(EffectId::SUMMON_SPIRIT)})
        if (MoveInfo move = tryEffect(effect, 1))
          return move;
    if (distance > 1) {
      if (distance < 10) {
        if (MoveInfo move = getFireMove(enemyDir))
          return move;
        if (MoveInfo move = getThrowMove(enemyDir))
          return move;
      }
      if (chase && !other->dontChase()) {
        lastSeen = none;
        if (auto action = creature->moveTowards(creature->getPosition() + enemyDir))
          return {max(0., 1.0 - double(distance) / 10), action.prepend([=] {
            GlobalEvents.addCombatEvent(creature);
            GlobalEvents.addCombatEvent(other);
            lastSeen = {creature->getPosition() + enemyDir, creature->getTime(), creature->getLevel(),
                LastSeen::ATTACK, other};
          })};
      }
    }
    if (distance == 1)
      if (auto action = creature->attack(other))
        return {1.0, action.prepend([=] {
            GlobalEvents.addCombatEvent(creature);
            GlobalEvents.addCombatEvent(other);
        })};
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(Fighter);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Behaviour)
      & SVAR(maxPowerRatio)
      & SVAR(chase)
      & SVAR(lastSeen);
    CHECK_SERIAL;
  }

  private:
  double SERIAL(maxPowerRatio);
  bool SERIAL(chase);
  struct LastSeen {
    Vec2 pos;
    double time;
    const Level* level;
    enum { ATTACK, PANIC} type;
    const Creature* creature;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
      ar& BOOST_SERIALIZATION_NVP(pos)
        & BOOST_SERIALIZATION_NVP(time)
        & BOOST_SERIALIZATION_NVP(level)
        & BOOST_SERIALIZATION_NVP(type)
        & BOOST_SERIALIZATION_NVP(creature);
    }
  };
  optional<LastSeen> SERIAL(lastSeen);
};

class GuardTarget : public Behaviour {
  public:
  GuardTarget(Creature* c, double minD, double maxD) : Behaviour(c), minDist(minD), maxDist(maxD) {}

  SERIALIZATION_CONSTRUCTOR(GuardTarget);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(minDist) & SVAR(maxDist);
    CHECK_SERIAL;
  }

  protected:
  MoveInfo getMoveTowards(Vec2 target) {
    double dist = (creature->getPosition() - target).lengthD();
    if (dist <= minDist)
      return NoMove;
    double exp = 1.5;
    double weight = pow((dist - minDist) / (maxDist - minDist), exp);
    if (auto action = creature->moveTowards(target))
      return {weight, action};
    else
      return NoMove;
  }

  private:
  double SERIAL(minDist);
  double SERIAL(maxDist);
};

class GuardArea : public Behaviour {
  public:
  GuardArea(Creature* c, const Location* l) : Behaviour(c), location(l) {}

  virtual MoveInfo getMove() override {
    if (auto action = creature->stayIn(location))
      return {1.0, action};
    else
      return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(GuardArea);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(location);
    CHECK_SERIAL;
  }

  private:
  const Location* SERIAL(location);
};

class GuardSquare : public GuardTarget {
  public:
  GuardSquare(Creature* c, Vec2 _pos, double minDist, double maxDist) : GuardTarget(c, minDist, maxDist), pos(_pos) {}

  virtual MoveInfo getMove() override {
    return getMoveTowards(pos);
  }

  SERIALIZATION_CONSTRUCTOR(GuardSquare);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(GuardTarget) & SVAR(pos);
    CHECK_SERIAL;
  }

  private:
  Vec2 SERIAL(pos);
};

class Wait : public Behaviour {
  public:
  Wait(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    return {1.0, creature->wait()};
  }

  SERIALIZATION_CONSTRUCTOR(Wait);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour);
  }
};

class DieTime : public Behaviour {
  public:
  DieTime(Creature* c, double t) : Behaviour(c), dieTime(t) {}

  virtual MoveInfo getMove() override {
    if (creature->getTime() > dieTime) {
      return {1.0, CreatureAction([=] {
        creature->die(nullptr, false, false);
      })};
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(DieTime);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Behaviour)
      & SVAR(dieTime);
    CHECK_SERIAL;
  }

  private:
  double SERIAL(dieTime);
};

class Summoned : public GuardTarget {
  public:
  Summoned(Creature* c, Creature* _target, double minDist, double maxDist, double ttl) 
      : GuardTarget(c, minDist, maxDist), target(_target), dieTime(target->getTime() + ttl) {
  }

  virtual ~Summoned() {
  }

  REGISTER_HANDLER(ChangeLevelEvent, const Creature* c, const Level* from, Vec2 pos, const Level* to, Vec2 toPos) {
    if (c == target)
      levelChanges[from] = pos;
  }

  virtual MoveInfo getMove() override {
    if (target->isDead() || creature->getTime() > dieTime) {
      return {1.0, CreatureAction([=] {
        creature->die(nullptr, false, false);
      })};
    }
    if (target->getLevel() == creature->getLevel()) {
      if (MoveInfo move = getMoveTowards(target->getPosition()))
        return move.setValue(0.5);
      else
        return NoMove;
    }
    if (!levelChanges.count(creature->getLevel()))
      return NoMove;
    Vec2 stairs = levelChanges.at(creature->getLevel());
    if (stairs == creature->getPosition() && creature->getSquare()->getApplyType(creature))
      return {0.5, creature->applySquare()};
    else
      return getMoveTowards(stairs);
  }

  SERIALIZATION_CONSTRUCTOR(Summoned);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(GuardTarget)
      & SVAR(target) 
      & SVAR(levelChanges)
      & SVAR(dieTime);
    CHECK_SERIAL;
  }

  private:
  Creature* SERIAL(target);
  map<const Level*, Vec2> SERIAL(levelChanges);
  double SERIAL(dieTime);
};

class Thief : public Behaviour {
  public:
  Thief(Creature* c) : Behaviour(c) {}
 
  virtual MoveInfo getMove() override {
    if (!creature->hasSkill(Skill::get(SkillId::STEALING)))
      return NoMove;
    for (const Creature* other : robbed) {
      if (creature->canSee(other)) {
        if (MoveInfo teleMove = tryEffect(EffectId::TELEPORT, 1))
          return teleMove;
        if (auto action = creature->moveAway(other->getPosition()))
        return {1.0, action};
      }
    }
    for (Square* square : creature->getSquares(Vec2::directions8(true))) {
      const Creature* other = square->getCreature();
      if (other && !contains(robbed, other)) {
        vector<Item*> allGold;
        for (Item* it : other->getEquipment().getItems())
          if (it->getClass() == ItemClass::GOLD)
            allGold.push_back(it);
        if (allGold.size() > 0)
          if (auto action = creature->stealFrom(other->getPosition() - creature->getPosition(), allGold))
          return {1.0, action.append([=] {
            other->playerMessage(creature->getName().the() + " steals all your gold!");
            robbed.push_back(other);
          })};
      }
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(Thief);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(robbed);
    CHECK_SERIAL;
  }

  private:
  vector<const Creature*> SERIAL(robbed);
};

class ByCollective : public Behaviour {
  public:
  ByCollective(Creature* c, Collective* col) : Behaviour(c), collective(col) {}

  virtual MoveInfo getMove() override {
    return collective->getMove(creature);
  }

  SERIALIZATION_CONSTRUCTOR(ByCollective);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(collective);
    CHECK_SERIAL;
  }

  private:
  Collective* SERIAL(collective);
};

class ChooseRandom : public Behaviour {
  public:
  ChooseRandom(Creature* c, vector<Behaviour*> beh, vector<double> w) : Behaviour(c), behaviours(beh), weights(w) {}

  virtual MoveInfo getMove() override {
    return chooseRandom(behaviours, weights)->getMove();
  }

  SERIALIZATION_CONSTRUCTOR(ChooseRandom);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(behaviours) & SVAR(weights);
    CHECK_SERIAL;
  }

  private:
  vector<Behaviour*> SERIAL(behaviours);
  vector<double> SERIAL(weights);
};

class SingleTask : public Behaviour {
  public:
  SingleTask(Creature* c, PTask t) : Behaviour(c), task(std::move(t)) {}

  virtual MoveInfo getMove() override {
    return task->getMove(creature);
  };

  SERIALIZATION_CONSTRUCTOR(SingleTask);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour) & SVAR(task);
    CHECK_SERIAL;
  }

  private:
  PTask SERIAL(task);
};

const static Vec2 splashTarget = Level::getSplashBounds().middle() - Vec2(3, 0);
const static Vec2 splashLeaderPos = Vec2(Level::getSplashVisibleBounds().getKX(),
    Level::getSplashBounds().middle().y) - Vec2(4, 0);

class SplashHeroes : public Behaviour {
  public:
  SplashHeroes(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->setCourage(100);
    if (!started && creature->getLevel()->getSafeSquare(splashLeaderPos)->getCreature())
      started = true;
    if (!started)
      return creature->wait();
    else {
      if (creature->getPosition().x > splashTarget.x)
        return {0.1, creature->move(Vec2(-1, 0))};
      else
        return {0.1, creature->wait()};
    }
  };

  SERIALIZATION_CONSTRUCTOR(SplashHeroes);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour);
    CHECK_SERIAL;
  }

  private:
  bool started = false;
};

class SplashHeroLeader : public Behaviour {
  public:
  SplashHeroLeader(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->setCourage(100);
    Vec2 pos = creature->getPosition();
    if (started)
      return creature->moveTowards(splashTarget);
    if (pos == splashLeaderPos)
      for (Square* square : creature->getSquares(
            {Vec2(2, 0), Vec2(2, -1), Vec2(2, 1), Vec2(3, 0), Vec2(3, -1), Vec2(3, 1)}))
        if (square->getCreature())
          started = true;
    if (pos != splashLeaderPos) {
      if (pos.y == splashLeaderPos.y)
        return creature->move(Vec2(-1, 0));
      else
        return creature->moveTowards(splashLeaderPos);
    } else
      return creature->wait();
  };

  SERIALIZATION_CONSTRUCTOR(SplashHeroLeader);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour);
    CHECK_SERIAL;
  }

  private:

  bool started = false;
};

class SplashMonsters : public Behaviour {
  public:
  SplashMonsters(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->setCourage(100);
    if (!initialPos)
      initialPos = creature->getPosition();
    vector<Creature*> heroes;
    for (Square* square : creature->getLevel()->getSquares(creature->getLevel()->getBounds().getAllSquares()))
      if (square->getCreature() && square->getCreature()->isEnemy(creature))
        heroes.push_back(square->getCreature());
    if (heroes.empty()) {
      if (creature->getPosition() == *initialPos)
        return creature->wait();
      else
        return creature->moveTowards(*initialPos);
    }
    if (const Creature* other = creature->getLevel()->getSafeSquare(splashTarget)->getCreature())
      if (creature->isEnemy(other))
        attack = true;
    if (!attack)
      return creature->wait();
    else
      return {0.1, creature->moveTowards(chooseRandom(heroes)->getPosition())};
  };

  SERIALIZATION_CONSTRUCTOR(SplashMonsters);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour);
    CHECK_SERIAL;
  }

  private:
  optional<Vec2> initialPos;
  bool attack = false;
};

class SplashItems : public Task::Callback {
  public:
  void addItems(Vec2 pos, vector<Item*> v) {
    items[pos] = v;
  }

  virtual void onBrought(Vec2 pos, EntitySet<Item>) override {
    removeElementMaybe(targetsGold, pos);
    removeElementMaybe(targetsCorpse, pos);
  }

  Vec2 chooseClosest(Vec2 pos) {
    Vec2 ret(1000, 1000);
    for (Vec2 v : getKeys(items))
      if (v.dist8(pos) < ret.dist8(pos))
        ret = v;
    return ret;
  }

  PTask getNextTask(Vec2 position) {
    if (items.empty())
      return nullptr;
    Vec2 pos = chooseClosest(position);
    vector<Item*> it = {chooseRandom(items[pos])};
    if (it[0]->getClass() == ItemClass::GOLD || it[0]->getClass() == ItemClass::AMMO)
      for (Item* it2 : copyOf(items[pos]))
        if (it[0] != it2 && it2->getClass() == it[0]->getClass() && Random.roll(10))
          it.push_back(it2);
    for (Item* it2 : it)
      removeElement(items[pos], it2);
    if (items[pos].empty())
      items.erase(pos);
    vector<Vec2>& targets = it[0]->getClass() == ItemClass::GOLD ? targetsGold : targetsCorpse;
    if (targets.empty())
      return nullptr;
    return Task::bringItem(this, pos, it, {chooseRandom(targets)});
  }

  void setInitialized() {
    initialized = true;
    ifstream iff("splash.txt");
    CHECK(!!iff);
    Vec2 sz;
    iff >> sz.x >> sz.y;
    for (int i : Range(sz.y)) {
      string s;
      iff >> s;
      for (int j : Range(sz.x)) {
        if (s[j] == '1')
          targetsGold.push_back(Level::getSplashVisibleBounds().getTopLeft() + Vec2(j, i));
        else if (s[j] == '2')
          targetsCorpse.push_back(Level::getSplashVisibleBounds().getTopLeft() + Vec2(j, i));
      }
    }
  }

  bool isInitialized() {
    return initialized;
  }

  private:
  map<Vec2, vector<Item*>> items;
  bool initialized = false;
  vector<Vec2> targetsGold;
  vector<Vec2> targetsCorpse;
} splashItems;

class SplashImps : public Behaviour {
  public:
  SplashImps(Creature* c) : Behaviour(c) {}

  void initializeSplashItems() {
    for (Vec2 v : Level::getSplashVisibleBounds()) {
      vector<Item*> inv = creature->getLevel()->getSafeSquare(v)->getItems(
          [](const Item* it) { return it->getClass() == ItemClass::GOLD || it->getClass() == ItemClass::CORPSE;});
      if (!inv.empty())
        splashItems.addItems(v, inv);
    }
    splashItems.setInitialized();
  }

  virtual MoveInfo getMove() override {
    creature->addEffect(LastingEffect::SPEED, 1000);
    if (!initialPos)
      initialPos = creature->getPosition();
    bool heroesDead = true;
    for (Square* square : creature->getLevel()->getSquares(Level::getSplashBounds().getAllSquares()))
      if (square->getCreature() && square->getCreature()->isEnemy(creature)) {
        heroesDead = false;
      }
    if (heroesDead) {
      for (Square* square : creature->getLevel()->getSquares(Level::getSplashVisibleBounds().getAllSquares()))
        if (square->getCreature() && square->getCreature()->getName().bare() != "imp")
          return creature->wait();
      if (!splashItems.isInitialized())
        initializeSplashItems();
      if (task) {
        if (!task->isDone())
          return task->getMove(creature);
        else
          task.reset();
      }
      task = splashItems.getNextTask(creature->getPosition());
      if (!task)
        return creature->moveTowards(*initialPos);
      else
        return task->getMove(creature);
    }
    return creature->wait();
  }

  SERIALIZATION_CONSTRUCTOR(SplashImps);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Behaviour);
    CHECK_SERIAL;
  }

  private:
  optional<Vec2> initialPos;
  PTask task;
};

template <class Archive>
void MonsterAI::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, Heal);
  REGISTER_TYPE(ar, Rest);
  REGISTER_TYPE(ar, MoveRandomly);
  REGISTER_TYPE(ar, AttackPest);
  REGISTER_TYPE(ar, BirdFlyAway);
  REGISTER_TYPE(ar, GoldLust);
  REGISTER_TYPE(ar, Fighter);
  REGISTER_TYPE(ar, GuardTarget);
  REGISTER_TYPE(ar, GuardArea);
  REGISTER_TYPE(ar, GuardSquare);
  REGISTER_TYPE(ar, Summoned);
  REGISTER_TYPE(ar, DieTime);
  REGISTER_TYPE(ar, Wait);
  REGISTER_TYPE(ar, Thief);
  REGISTER_TYPE(ar, ByCollective);
  REGISTER_TYPE(ar, ChooseRandom);
  REGISTER_TYPE(ar, SingleTask);
}

REGISTER_TYPES(MonsterAI);

MonsterAI::MonsterAI(Creature* c, const vector<Behaviour*>& beh, const vector<int>& w, bool pick) :
    weights(w), creature(c), pickItems(pick) {
  CHECK(beh.size() == w.size());
  for (auto b : beh)
    behaviours.push_back(PBehaviour(b));
}

void MonsterAI::makeMove() {
  vector<pair<MoveInfo, int>> moves;
  for (int i : All(behaviours)) {
    MoveInfo move = behaviours[i]->getMove();
    move.value *= weights[i];
    moves.emplace_back(move, weights[i]);
    if (pickItems) {
      for (auto elem : Item::stackItems(creature->getPickUpOptions())) {
        Item* item = elem.second[0];
        if (!item->getShopkeeper() && creature->pickUp(elem.second))
          moves.emplace_back(
              MoveInfo({ behaviours[i]->itemValue(item) * weights[i], creature->pickUp(elem.second)}),
              weights[i]);
      }
    }
  }
  /*vector<Item*> inventory = creature->getEquipment().getItems([this](Item* item) { return !creature->getEquipment().isEquiped(item);});
  for (Item* item : inventory) {
    bool useless = true;
    for (PBehaviour& behaviour : behaviours)
      if (behaviour->itemValue(item) > 0)
        useless = false;
    if (useless)
      moves.push_back({ 0.01, [=]() {
        creature->drop({item});
      }});
  }*/
  MoveInfo winner = NoMove;
  for (int i : All(moves)) {
    MoveInfo& move = moves[i].first;
    if (move.value > winner.value)
      winner = move;
    if (i < moves.size() - 1 && move.value > moves[i + 1].second)
      break;
  }
  CHECK(winner.value > 0);
  winner.move.perform();
}

PMonsterAI MonsterAIFactory::getMonsterAI(Creature* c) {
  return PMonsterAI(maker(c));
}

MonsterAIFactory::MonsterAIFactory(MakerFun _maker) : maker(_maker) {
}

MonsterAIFactory MonsterAIFactory::monster() {
  return stayInLocation(nullptr);
}

MonsterAIFactory MonsterAIFactory::collective(Collective* col) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
        new Heal(c, false),
        new Fighter(c, 0.6, true),
        new ByCollective(c, col),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c, 3)}, {3, 1}),
        new AttackPest(c)},
        { 6, 5, 2, 1, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::stayInLocation(Location* l, bool moveRandomly) {
  return MonsterAIFactory([=](Creature* c) {
      vector<Behaviour*> actors { 
          new Heal(c),
          new Thief(c),
          new Fighter(c, 0.6, true),
          new AttackPest(c),
          new GoldLust(c)};
      vector<int> weights { 5, 4, 3, 1, 1 };
      if (l != nullptr) {
        actors.push_back(new GuardArea(c, l));
        weights.push_back(1);
      }
      if (moveRandomly) {
        actors.push_back(new MoveRandomly(c, 3));
        weights.push_back(1);
      } else {
        actors.push_back(new Wait(c));
        weights.push_back(1);
      }
      return new MonsterAI(c, actors, weights);
  });
}

MonsterAIFactory MonsterAIFactory::singleTask(PTask& task) {
  return MonsterAIFactory([=, &task](Creature* c) {
      return new MonsterAI(c, {
        new Heal(c, false),
        new Fighter(c, 0.6, false),
        new SingleTask(c, std::move(task)),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c, 3)}, {3, 1})},
        { 6, 5, 2, 1}, true);
      });
}

MonsterAIFactory MonsterAIFactory::wildlifeNonPredator() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new Fighter(c, 1.2, false),
          new MoveRandomly(c, 3)},
          {5, 1});
      });
}

MonsterAIFactory MonsterAIFactory::moveRandomly() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new MoveRandomly(c, 3)},
          {1});
      });
}

MonsterAIFactory MonsterAIFactory::idle() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new Rest(c)},
          {1});
      });
}

MonsterAIFactory MonsterAIFactory::scavengerBird(Vec2 corpsePos) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
          new BirdFlyAway(c, 3),
          new MoveRandomly(c, 3),
          new GuardSquare(c, corpsePos, 1, 2)},
          {1, 1, 2});
      });
}

MonsterAIFactory MonsterAIFactory::guardSquare(Vec2 pos) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
          new Wait(c),
          new GuardSquare(c, pos, 0, 1)},
          {1, 2});
      });
}

MonsterAIFactory MonsterAIFactory::summoned(Creature* leader, int ttl) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
          new Summoned(c, leader, 1, 3, ttl),
          new Heal(c),
          new Fighter(c, 0.6, true),
          new MoveRandomly(c, 3),
          new GoldLust(c)},
          { 5, 4, 3, 1, 1 });
      });
}

MonsterAIFactory MonsterAIFactory::dieTime(double dieTime) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
          new DieTime(c, dieTime),
          new Heal(c),
          new Fighter(c, 0.6, true),
          new MoveRandomly(c, 3),
          new GoldLust(c)},
          { 5, 4, 3, 1, 1 });
      });
}

MonsterAIFactory MonsterAIFactory::splashHeroes(bool leader) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
        leader ? (Behaviour*)new SplashHeroLeader(c) : (Behaviour*)new SplashHeroes(c),
        new Heal(c, false),
        new Fighter(c, 0.6, true),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c, 3)}, {3, 1}),
        new AttackPest(c)},
        { 6, 5, 2, 1, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::splashMonsters(bool imps) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
        imps ? (Behaviour*)new SplashImps(c) : (Behaviour*)new SplashMonsters(c),
        new Heal(c, false),
        new Fighter(c, 0.6, true),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c, 3)}, {3, 1}),
        new AttackPest(c)},
        { 6, 5, 2, 1, 1}, false);
      });
}

