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
#include "level.h"
#include "collective.h"
#include "effect.h"
#include "item.h"
#include "creature.h"
#include "location.h"
#include "player_message.h"
#include "equipment.h"
#include "spell.h"
#include "creature_name.h"
#include "skill.h"
#include "modifier_type.h"
#include "task.h"
#include "game.h"
#include "creature_attributes.h"
#include "creature_factory.h"
#include "spell_map.h"
#include "effect_type.h"
#include "body.h"
#include "item_class.h"
#include "furniture.h"

class Behaviour {
  public:
  Behaviour(Creature*);
  virtual MoveInfo getMove() { return NoMove; }
  virtual void onAttacked(const Creature* attacker) {}
  virtual double itemValue(const Item*) { return 0; }
  Item* getBestWeapon();
  Creature* getClosestEnemy();
  Creature* getClosestCreature();
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
}

SERIALIZABLE(MonsterAI);

SERIALIZATION_CONSTRUCTOR_IMPL(MonsterAI);

template <class Archive> 
void Behaviour::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(creature);
}


Behaviour::Behaviour(Creature* c) : creature(c) {
}

SERIALIZATION_CONSTRUCTOR_IMPL(Behaviour);

Creature* Behaviour::getClosestEnemy() {
  int dist = 1000000000;
  Creature* result = nullptr;
  for (Creature* other : creature->getVisibleEnemies()) {
    int curDist = other->getPosition().dist8(creature->getPosition());
    if (curDist < dist && (!other->getAttributes().dontChase() || curDist == 1)) {
      result = other;
      dist = creature->getPosition().dist8(other->getPosition());
    }
  }
  return result;
}

Creature* Behaviour::getClosestCreature() {
  int dist = 1000000000;
  Creature* result = nullptr;
  for (Creature* other : creature->getVisibleCreatures())
    if (other != creature && other->getPosition().dist8(creature->getPosition()) < dist) {
      result = other;
      dist = creature->getPosition().dist8(other->getPosition());
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
  for (Spell* spell : creature->getAttributes().getSpellMap().getAll()) {
   if (spell->hasEffect(type))
      if (auto action = creature->castSpell(spell))
        return { 1, action };
  }
  auto items = creature->getEquipment().getItems(Item::effectPredicate(type)); 
  for (Item* item : items)
    if (item->getApplyTime() <= maxTurns)
      if (auto action = creature->applyItem(item))
        return MoveInfo(1, action);
  return NoMove;
}

MoveInfo Behaviour::tryEffect(DirEffectType type, Vec2 dir) {
  for (Spell* spell : creature->getAttributes().getSpellMap().getAll()) {
    if (spell->hasEffect(type))
      if (auto action = creature->castSpell(spell, dir))
        return { 1, action };
  }
  return NoMove;
}

class Heal : public Behaviour {
  public:
  Heal(Creature* c) : Behaviour(c) {}

  virtual double itemValue(const Item* item) {
    if (item->getEffectType() == EffectType(EffectId::HEAL)) {
      return 0.5;
    }
    else
      return 0;
  }

  virtual MoveInfo getMove() {
    if (creature->getAttributes().getSkills().hasDiscrete(SkillId::HEALING)) {
      int healingRadius = 2;
      for (Position pos : creature->getPosition().getRectangle(
            Rectangle(-healingRadius, -healingRadius, healingRadius + 1, healingRadius + 1)))
        if (const Creature* other = pos.getCreature())
          if (creature->isFriend(other))
            if (auto action = creature->heal(creature->getPosition().getDir(other->getPosition())))
              return MoveInfo(0.5, action);
    }
    if (!creature->getBody().isHumanoid())
      return NoMove;
    if (creature->isAffected(LastingEffect::POISON)) {
      if (MoveInfo move = tryEffect(EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT), 1))
        return move;
      if (MoveInfo move = tryEffect(EffectType(EffectId::CURE_POISON), 1))
        return move;
    }
    if (creature->getBody().canHeal()) {
      if (MoveInfo move = tryEffect(EffectId::HEAL, 1))
        return move.withValue(min(1.0, 1.5 - creature->getBody().getHealth()));
      if (MoveInfo move = tryEffect(EffectId::HEAL, 3))
        return move.withValue(0.5 * min(1.0, 1.5 - creature->getBody().getHealth()));
    }
    for (Position pos : creature->getPosition().neighbors8())
      if (Creature* c = pos.getCreature())
        if (creature->isFriend(c) && c->getBody().canHeal())
          for (Item* item : creature->getEquipment().getItems(Item::effectPredicate(EffectId::HEAL)))
            if (auto action = creature->give(c, {item}))
              return MoveInfo(0.5, action);
    return NoMove;
  }


  SERIALIZE_SUBCLASS(Behaviour);
  SERIALIZATION_CONSTRUCTOR(Heal);
};

class Rest : public Behaviour {
  public:
  Rest(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() {
    return {0.1, creature->wait() };
  }

  SERIALIZATION_CONSTRUCTOR(Rest);
  SERIALIZE_SUBCLASS(Behaviour);
};

class MoveRandomly : public Behaviour {
  public:
  MoveRandomly(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() {
    if (!visited(creature->getPosition()))
      updateMem(creature->getPosition());
    optional<Position> target;
    double val = 0.0001;
    if (Random.roll(2))
      return {val, creature->wait()};
    for (Position pos : creature->getPosition().neighbors8(Random)) {
      if (!visited(pos) && creature->move(pos)) {
        target = pos;
        break;
      }
    }
    if (!target)
      for (Position pos: creature->getPosition().neighbors8(Random))
        if (creature->move(pos)) {
          target = pos;
          break;
        }
    if (!target)
      return {val, creature->wait() };
    else
      return {val, creature->move(*target).append([=] (Creature* creature) {
          updateMem(creature->getPosition());
      })};
  }

  void updateMem(Position pos) {
    memory.push_back(pos);
    if (memory.size() > memSize)
      memory.pop_front();
  }

  bool visited(Position pos) {
    return contains(memory, pos);
  }

  SERIALIZATION_CONSTRUCTOR(MoveRandomly);
  SERIALIZE_SUBCLASS(Behaviour);

  private:
  deque<Position> memory;
  const int memSize = 3;
};

class StayOnFurniture : public Behaviour {
  public:
  StayOnFurniture(Creature* c, FurnitureType t) : Behaviour(c), type(t) {}

  MoveInfo getMove() override {
    auto furniture = creature->getPosition().getFurniture();
    if (!furniture || furniture->getType() != type) {
      if (!nextPigsty)
        for (auto pos : creature->getPosition().getRectangle(Rectangle::centered(Vec2(0, 0), 20)))
          if (Furniture* f = pos.getFurniture())
            if (f->getType() == type) {
              nextPigsty = pos;
              break;
            }
      if (nextPigsty)
        if (auto move = creature->moveTowards(*nextPigsty, true))
          return move;
    }
    if (Random.roll(10))
      for (Position next: creature->getPosition().neighbors8(Random))
        if (next.canEnter(creature) && next.getFurniture() && next.getFurniture()->getType() == type)
          return creature->move(next);
    return creature->wait();
  }

  SERIALIZATION_CONSTRUCTOR(StayOnFurniture)
  SERIALIZE_ALL2(Behaviour, type)

  private:
  FurnitureType SERIAL(type);
  optional<Position> nextPigsty;
};

class BirdFlyAway : public Behaviour {
  public:
  BirdFlyAway(Creature* c, double _maxDist) : Behaviour(c), maxDist(_maxDist) {}

  virtual MoveInfo getMove() override {
    const Creature* enemy = getClosestEnemy();
    if (Random.roll(15) || ( enemy && enemy->getPosition().dist8(creature->getPosition()) < maxDist))
      if (auto action = creature->flyAway())
        return {1.0, action};
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(BirdFlyAway);
  SERIALIZE_ALL2(Behaviour, maxDist);

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
  SERIALIZE_SUBCLASS(Behaviour);
};

class Wildlife : public Behaviour {
  public:
  Wildlife(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    if (Creature* other = getClosestCreature()) {
      int dist = creature->getPosition().dist8(other->getPosition());
      if (dist == 1)
        return creature->attack(other);
      if (dist < 7)
        // pathfinding is expensive so only do it when running away from the player
        return creature->moveAway(other->getPosition(), other->isPlayer());
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(Wildlife);
  SERIALIZE_SUBCLASS(Behaviour);
};

class Fighter : public Behaviour {
  public:
  Fighter(Creature* c, double powerR, bool _chase) : Behaviour(c), maxPowerRatio(powerR), chase(_chase) {
  }

  virtual ~Fighter() {
  }

  double getMoraleBonus() {
    return creature->getAttributes().getCourage() * pow(2.0, creature->getMorale());
  }

  virtual MoveInfo getMove() override {
    if (Creature* other = getClosestEnemy()) {
      double myDamage = creature->getModifier(ModifierType::DAMAGE);
      Item* weapon = getBestWeapon();
      if (!creature->getWeapon() && weapon)
        myDamage += weapon->getModifier(ModifierType::DAMAGE);
      double powerRatio = getMoraleBonus() * myDamage / other->getModifier(ModifierType::DAMAGE);
      bool significantEnemy = myDamage < 5 * other->getModifier(ModifierType::DAMAGE);
      double panicWeight = 0.1;
      if (creature->getBody().isWounded())
        panicWeight += 0.4;
      if (creature->getBody().isSeriouslyWounded())
        panicWeight += 0.5;
      if (powerRatio < maxPowerRatio)
        panicWeight += 2 - powerRatio * 2;
      panicWeight = min(1.0, max(0.0, panicWeight));
      if (creature->isAffected(LastingEffect::PANIC))
        panicWeight = 1;
      if (other->hasCondition(CreatureCondition::SLEEPING))
        panicWeight = 0;
      Debug() << creature->getName().bare() << " panic weight " << panicWeight;
      if (panicWeight >= 0.5) {
        double dist = creature->getPosition().dist8(other->getPosition());
        if (dist < 7) {
          if (dist == 1 && creature->getBody().isHumanoid())
            creature->surrender(other);
          if (MoveInfo move = getPanicMove(other, panicWeight))
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

  MoveInfo getPanicMove(Creature* other, double weight) {
    if (auto teleMove = tryEffect(EffectId::TELEPORT, 1))
      return teleMove.withValue(weight);
    if (other->getPosition().dist8(creature->getPosition()) > 3)
      if (auto move = getFireMove(creature->getPosition().getDir(other->getPosition())))
        return move.withValue(weight);
    if (auto action = creature->moveAway(other->getPosition(), chase))
      return {weight, action.prepend([=](Creature* creature) {
        creature->setInCombat();
        other->setInCombat();
        lastSeen = {creature->getPosition(), creature->getGlobalTime(), LastSeen::PANIC, other->getUniqueId()};
      })};
    else
      return NoMove;
  }

  virtual double itemValue(const Item* item) override {
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
    if (item->getClass() == ItemClass::AMMO && creature->getAttributes().getSkills().getValue(SkillId::ARCHERY) > 0)
      return 0.1;
    if (!creature->isEquipmentAppropriate(item))
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
      const Creature* c = creature->getPosition().plus(v).getCreature();
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
        return !creature->getEquipment().isEquipped(item);});
    for (Item* item : items)
      if (getThrowValue(item) > damage) {
        damage = getThrowValue(item);
        best = item;
      }
    if (best)
      if (auto action = creature->throwItem(best, dir)) {
        creature->setInCombat();
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
      return {1.0, action.append([=](Creature* creature) {
          creature->setInCombat();
      })};
    return NoMove;
  }

  MoveInfo getLastSeenMove() {
    if (auto lastSeen = getLastSeen()) {
      double lastSeenTimeout = 20;
      if (!lastSeen->pos.isSameLevel(creature->getPosition()) ||
          lastSeen->time < creature->getGlobalTime() - lastSeenTimeout ||
          lastSeen->pos == creature->getPosition()) {
        lastSeen = none;
        return NoMove;
      }
      if (chase && lastSeen->type == LastSeen::ATTACK)
        if (auto action = creature->moveTowards(lastSeen->pos)) {
          return {0.5, action.append([=](Creature* creature) {
              creature->setInCombat();
              })};
        }
      if (lastSeen->type == LastSeen::PANIC && lastSeen->pos.dist8(creature->getPosition()) < 4)
        if (auto action = creature->moveAway(lastSeen->pos, chase))
          return {0.5, action.append([=](Creature* creature) {
              creature->setInCombat();
              })};
    }
    return NoMove;
  }

  MoveInfo getAttackMove(Creature* other, bool chase) {
    int distance = 10000;
    CHECK(other);
    if (other->getAttributes().isBoulder())
      return NoMove;
    Debug() << creature->getName().bare() << " enemy " << other->getName().bare();
    Vec2 enemyDir = creature->getPosition().getDir(other->getPosition());
    distance = enemyDir.length8();
    if (creature->getBody().isHumanoid() && !creature->getWeapon()) {
      if (Item* weapon = getBestWeapon())
        if (auto action = creature->equip(weapon))
          return {3.0 / (2.0 + distance), action.prepend([=](Creature* creature) {
            creature->setInCombat();
            other->setInCombat();
        })};
    }
    if (distance == 1)
      if (MoveInfo move = tryEffect(EffectId::AIR_BLAST, 1))
        return move;
    if (distance <= 5)
      for (EffectType effect : {
          EffectType(EffectId::LASTING, LastingEffect::INVISIBLE),
          EffectType(EffectId::LASTING, LastingEffect::STR_BONUS),
          EffectType(EffectId::LASTING, LastingEffect::DEX_BONUS),
          EffectType(EffectId::LASTING, LastingEffect::SPEED),
          EffectType(EffectId::DECEPTION),
          EffectType(EffectId::SUMMON, CreatureId::SPIRIT)})
        if (MoveInfo move = tryEffect(effect, 1))
          return move;
    if (distance > 1) {
      if (distance < 10) {
        if (MoveInfo move = getFireMove(enemyDir))
          return move;
        if (MoveInfo move = getThrowMove(enemyDir))
          return move;
      }
      if (chase && !other->getAttributes().dontChase() && !isChaseFrozen(other)) {
        lastSeen = none;
        if (auto action = creature->moveTowards(other->getPosition()))
          return {max(0., 1.0 - double(distance) / 10), action.prepend([=](Creature* creature) {
            creature->setInCombat();
            other->setInCombat();
            lastSeen = {other->getPosition(), creature->getGlobalTime(), LastSeen::ATTACK, other->getUniqueId()};
            if (!chaseFreeze.count(other) || other->getGlobalTime() > chaseFreeze.at(other).second)
              chaseFreeze[other] = make_pair(other->getGlobalTime() + 20, other->getGlobalTime() + 70);
          })};
      }
    }
    if (distance == 1)
      if (auto action = creature->attack(other, getAttackParams(other)))
        return {1.0, action.prepend([=](Creature* creature) {
            creature->setInCombat();
            other->setInCombat();
        })};
    return NoMove;
  }

  Creature::AttackParams getAttackParams(const Creature* enemy) {
    int damDiff = enemy->getModifier(ModifierType::DAMAGE) - creature->getModifier(ModifierType::DAMAGE);
    if (damDiff > 10)
      return CONSTRUCT(Creature::AttackParams, c.mod = Creature::AttackParams::WILD;);
    else if (damDiff < -10)
      return CONSTRUCT(Creature::AttackParams, c.mod = Creature::AttackParams::SWIFT;);
    else
      return Creature::AttackParams {};
  }

  SERIALIZATION_CONSTRUCTOR(Fighter);
  SERIALIZE_ALL2(Behaviour, maxPowerRatio, chase, lastSeen);

  private:
  double SERIAL(maxPowerRatio);
  bool SERIAL(chase);
  struct LastSeen {
    Position SERIAL(pos);
    double SERIAL(time);
    enum { ATTACK, PANIC} SERIAL(type);
    Creature::Id SERIAL(creature);
    SERIALIZE_ALL(pos, time, type, creature);
  };
  optional<LastSeen> SERIAL(lastSeen);
  optional<LastSeen>& getLastSeen() {
    if (lastSeen && !creature->getLevel()->containsCreature(lastSeen->creature))
      lastSeen.reset();
    return lastSeen;
  }
  map<const Creature*, pair<double, double>> chaseFreeze;

  bool isChaseFrozen(const Creature* c) {
    return chaseFreeze.count(c) && chaseFreeze.at(c).first <= c->getGlobalTime()
      && chaseFreeze.at(c).second >= c->getGlobalTime();
  }
};

class GuardTarget : public Behaviour {
  public:
  GuardTarget(Creature* c, double minD, double maxD) : Behaviour(c), minDist(minD), maxDist(maxD) {}

  SERIALIZATION_CONSTRUCTOR(GuardTarget);
  SERIALIZE_ALL2(Behaviour, minDist, maxDist);

  protected:
  MoveInfo getMoveTowards(Position target) {
    double dist = creature->getPosition().dist8(target);
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
  SERIALIZE_ALL2(Behaviour, location);

  private:
  const Location* SERIAL(location);
};

class GuardSquare : public GuardTarget {
  public:
  GuardSquare(Creature* c, Position _pos, double minDist, double maxDist) : GuardTarget(c, minDist, maxDist),
      pos(_pos) {}

  virtual MoveInfo getMove() override {
    return getMoveTowards(pos);
  }

  SERIALIZATION_CONSTRUCTOR(GuardSquare);
  SERIALIZE_ALL2(GuardTarget, pos);

  private:
  Position SERIAL(pos);
};

class Wait : public Behaviour {
  public:
  Wait(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    return {1.0, creature->wait()};
  }

  SERIALIZATION_CONSTRUCTOR(Wait);
  SERIALIZE_SUBCLASS(Behaviour);
};

class DieTime : public Behaviour {
  public:
  DieTime(Creature* c, double t) : Behaviour(c), dieTime(t) {}

  virtual MoveInfo getMove() override {
    if (creature->getGlobalTime() > dieTime) {
      return {1.0, CreatureAction(creature, [=](Creature* creature) {
        creature->die(nullptr, false, false);
      })};
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(DieTime);
  SERIALIZE_ALL2(Behaviour, dieTime);

  private:
  double SERIAL(dieTime);
};

class Summoned : public GuardTarget {
  public:
  Summoned(Creature* c, Creature* _target, double minDist, double maxDist, double ttl) 
      : GuardTarget(c, minDist, maxDist), target(_target), dieTime(target->getGlobalTime() + ttl) {
  }

  virtual ~Summoned() {
  }

  virtual MoveInfo getMove() override {
    if (target->isDead() || creature->getGlobalTime() > dieTime) {
      return {1.0, CreatureAction(creature, [=](Creature* creature) {
        creature->die(nullptr, false, false);
      })};
    }
    if (MoveInfo move = getMoveTowards(target->getPosition()))
      return move.withValue(0.5);
    else
      return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(Summoned);
  SERIALIZE_ALL2(GuardTarget, target, dieTime);

  private:
  Creature* SERIAL(target);
  double SERIAL(dieTime);
};

class Thief : public Behaviour {
  public:
  Thief(Creature* c) : Behaviour(c) {}
 
  virtual MoveInfo getMove() override {
    if (!creature->getAttributes().getSkills().hasDiscrete(SkillId::STEALING))
      return NoMove;
    for (const Creature* other : creature->getVisibleEnemies()) {
      if (robbed.contains(other)) {
        if (MoveInfo teleMove = tryEffect(EffectId::TELEPORT, 1))
          return teleMove;
        if (auto action = creature->moveAway(other->getPosition()))
        return {1.0, action};
      }
    }
    for (Position pos : creature->getPosition().neighbors8(Random)) {
      const Creature* other = pos.getCreature();
      if (other && !robbed.contains(other)) {
        vector<Item*> allGold;
        for (Item* it : other->getEquipment().getItems())
          if (it->getClass() == ItemClass::GOLD)
            allGold.push_back(it);
        if (allGold.size() > 0)
          if (auto action = creature->stealFrom(creature->getPosition().getDir(other->getPosition()), allGold))
          return {1.0, action.append([=](Creature* creature) {
            other->playerMessage(creature->getName().the() + " steals all your gold!");
            robbed.insert(other);
          })};
      }
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(Thief);
  SERIALIZE_ALL2(Behaviour, robbed);

  private:
  EntitySet<Creature> SERIAL(robbed);
};

class ByCollective : public Behaviour {
  public:
  ByCollective(Creature* c, Collective* col) : Behaviour(c), collective(col) {}

  virtual MoveInfo getMove() override {
    return collective->getMove(creature);
  }

  SERIALIZATION_CONSTRUCTOR(ByCollective);
  SERIALIZE_ALL2(Behaviour, collective);

  private:
  Collective* SERIAL(collective);
};

class ChooseRandom : public Behaviour {
  public:
  ChooseRandom(Creature* c, vector<Behaviour*> beh, vector<double> w) : Behaviour(c), behaviours(beh), weights(w) {}

  virtual MoveInfo getMove() override {
    return Random.choose(behaviours, weights)->getMove();
  }

  SERIALIZATION_CONSTRUCTOR(ChooseRandom);
  SERIALIZE_ALL2(Behaviour, behaviours, weights);

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
  SERIALIZE_ALL2(Behaviour, task);

  private:
  PTask SERIAL(task);
};

const static Vec2 splashTarget = Level::getSplashBounds().middle() - Vec2(3, 0);
const static Vec2 splashLeaderPos = Vec2(Level::getSplashVisibleBounds().right(),
    Level::getSplashBounds().middle().y) - Vec2(4, 0);

class SplashHeroes : public Behaviour {
  public:
  SplashHeroes(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->getAttributes().setCourage(100);
    if (!started && creature->getPosition().withCoord(splashLeaderPos).getCreature())
      started = true;
    if (!started)
      return creature->wait();
    else {
      if (creature->getPosition().getCoord().x > splashTarget.x)
        return {0.1, creature->move(Vec2(-1, 0))};
      else
        return {0.1, creature->wait()};
    }
  };

  SERIALIZATION_CONSTRUCTOR(SplashHeroes);
  SERIALIZE_SUBCLASS(Behaviour);

  private:
  bool started = false;
};

class SplashHeroLeader : public Behaviour {
  public:
  SplashHeroLeader(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->getAttributes().setCourage(100);
    Vec2 pos = creature->getPosition().getCoord();
    if (started)
      return creature->moveTowards(creature->getPosition().withCoord(splashTarget));
    if (pos == splashLeaderPos)
      for (Vec2 v : {Vec2(2, 0), Vec2(2, -1), Vec2(2, 1), Vec2(3, 0), Vec2(3, -1), Vec2(3, 1)})
        if (creature->getPosition().plus(v).getCreature())
          started = true;
    if (pos != splashLeaderPos) {
      if (pos.y == splashLeaderPos.y)
        return creature->move(Vec2(-1, 0));
      else
        return creature->moveTowards(creature->getPosition().withCoord(splashLeaderPos));
    } else
      return creature->wait();
  };

  SERIALIZATION_CONSTRUCTOR(SplashHeroLeader);
  SERIALIZE_SUBCLASS(Behaviour);

  private:

  bool started = false;
};

class SplashMonsters : public Behaviour {
  public:
  SplashMonsters(Creature* c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->getAttributes().setCourage(100);
    if (!initialPos)
      initialPos = creature->getPosition().getCoord();
    vector<Creature*> heroes;
    for (Position v : creature->getLevel()->getAllPositions())
      if (v.getCreature() && v.getCreature()->isEnemy(creature))
        heroes.push_back(v.getCreature());
    if (heroes.empty()) {
      if (creature->getPosition().getCoord() == *initialPos)
        return creature->wait();
      else
        return creature->moveTowards(Position(*initialPos, creature->getLevel()));
    }
    if (const Creature* other = creature->getLevel()->getPosition(splashTarget).getCreature())
      if (creature->isEnemy(other))
        attack = true;
    if (!attack)
      return creature->wait();
    else
      return {0.1, creature->moveTowards(Random.choose(heroes)->getPosition())};
  };

  SERIALIZATION_CONSTRUCTOR(SplashMonsters);
  SERIALIZE_SUBCLASS(Behaviour);

  private:
  optional<Vec2> initialPos;
  bool attack = false;
};

class SplashItems : public TaskCallback {
  public:
  void addItems(Vec2 pos, vector<Item*> v) {
    items[pos] = v;
  }

  Vec2 chooseClosest(Vec2 pos) {
    Vec2 ret(1000, 1000);
    for (Vec2 v : getKeys(items))
      if (v.dist8(pos) < ret.dist8(pos))
        ret = v;
    return ret;
  }

  PTask getNextTask(Vec2 position, Level* level) {
    if (items.empty())
      return nullptr;
    Vec2 pos = chooseClosest(position);
    vector<Item*> it = {Random.choose(items[pos])};
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
    Vec2 target = Random.choose(targets);
    removeElement(targets, target);
    return Task::bringItem(this, Position(pos, level), it, {Position(target, level)}, 100);
  }

  void setInitialized(const string& splashPath) {
    initialized = true;
    ifstream iff(splashPath.c_str());
    CHECK(!!iff);
    Vec2 sz;
    iff >> sz.x >> sz.y;
    for (int i : Range(sz.y)) {
      string s;
      iff >> s;
      for (int j : Range(sz.x)) {
        if (s[j] == '1')
          targetsGold.push_back(Level::getSplashVisibleBounds().topLeft() + Vec2(j, i));
        else if (s[j] == '2')
          targetsCorpse.push_back(Level::getSplashVisibleBounds().topLeft() + Vec2(j, i));
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
  SplashImps(Creature* c, const string& splash) : Behaviour(c), splashPath(splash) {}

  void initializeSplashItems() {
    for (Vec2 v : Level::getSplashVisibleBounds()) {
      vector<Item*> inv = creature->getLevel()->getPosition(v).getItems(
          [](const Item* it) { return it->getClass() == ItemClass::GOLD || it->getClass() == ItemClass::CORPSE;});
      if (!inv.empty())
        splashItems.addItems(v, inv);
    }
    splashItems.setInitialized(splashPath);
  }

  virtual MoveInfo getMove() override {
    creature->addEffect(LastingEffect::SPEED, 1000);
    if (!initialPos)
      initialPos = creature->getPosition().getCoord();
    bool heroesDead = true;
    for (const Creature* c : creature->getLevel()->getAllCreatures())
      if (c->isEnemy(creature)) {
        heroesDead = false;
      }
    if (heroesDead) {
      if (!splashItems.isInitialized())
        initializeSplashItems();
      if (task) {
        if (!task->isDone())
          return task->getMove(creature);
        else
          task.reset();
      }
      task = splashItems.getNextTask(creature->getPosition().getCoord(), creature->getLevel());
      if (!task)
        return creature->moveTowards(Position(*initialPos, creature->getLevel()));
      else
        return task->getMove(creature);
    }
    return creature->wait();
  }

  SERIALIZATION_CONSTRUCTOR(SplashImps);
  SERIALIZE_SUBCLASS(Behaviour);

  private:
  optional<Vec2> initialPos;
  PTask task;
  string splashPath;
};

class AvoidFire : public Behaviour {
  public:
  using Behaviour::Behaviour;

  virtual MoveInfo getMove() override {
    if (creature->getPosition().isBurning() && !creature->isAffected(LastingEffect::FIRE_RESISTANT)) {
      for (Position pos : creature->getPosition().neighbors8(Random))
        if (!pos.isBurning())
          if (auto action = creature->move(pos))
            return action;
      for (Position pos : creature->getPosition().neighbors8(Random))
        if (auto action = creature->forceMove(pos))
          return action;
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(AvoidFire);
  SERIALIZE_SUBCLASS(Behaviour);
};

template <class Archive>
void MonsterAI::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, Heal);
  REGISTER_TYPE(ar, Rest);
  REGISTER_TYPE(ar, MoveRandomly);
  REGISTER_TYPE(ar, BirdFlyAway);
  REGISTER_TYPE(ar, GoldLust);
  REGISTER_TYPE(ar, Wildlife);
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
  REGISTER_TYPE(ar, AvoidFire);
  REGISTER_TYPE(ar, StayOnFurniture);
}

REGISTER_TYPES(MonsterAI::registerTypes);

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
    move.setValue(move.getValue() * weights[i]);
    moves.emplace_back(move, weights[i]);
    if (pickItems) {
      for (auto elem : Item::stackItems(creature->getPickUpOptions())) {
        Item* item = elem.second[0];
        if (!item->isOrWasForSale() && creature->pickUp(elem.second))
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
    if (move.getValue() > winner.getValue())
      winner = move;
    if (i < moves.size() - 1 && move.getValue() > moves[i + 1].second)
      break;
  }
  CHECK(winner.getValue() > 0);
  winner.getMove().perform(creature);
}

PMonsterAI MonsterAIFactory::getMonsterAI(Creature* c) const {
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
        new AvoidFire(c),
        new Heal(c),
        new Fighter(c, 0.6, true),
        new ByCollective(c, col),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c)}, {3, 1})},
        { 10, 6, 5, 2, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::stayInLocation(Location* l, bool moveRandomly) {
  return MonsterAIFactory([=](Creature* c) {
      vector<Behaviour*> actors { 
          new AvoidFire(c),
          new Heal(c),
          new Thief(c),
          new Fighter(c, 0.6, true),
          new GoldLust(c)};
      vector<int> weights { 10, 5, 4, 3, 1 };
      if (l != nullptr) {
        actors.push_back(new GuardArea(c, l));
        weights.push_back(1);
      }
      if (moveRandomly) {
        actors.push_back(new MoveRandomly(c));
        weights.push_back(1);
      } else {
        actors.push_back(new Wait(c));
        weights.push_back(1);
      }
      return new MonsterAI(c, actors, weights);
  });
}

MonsterAIFactory MonsterAIFactory::singleTask(PTask&& t) {
  Task* released = t.release();
  return MonsterAIFactory([=](Creature* c) mutable {
      CHECK(released);
      Task* task = released;
      released = nullptr;
      return new MonsterAI(c, {
        new Heal(c),
        new Fighter(c, 0.6, false),
        new SingleTask(c, PTask(task)),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c)}, {3, 1})},
        { 6, 5, 2, 1}, true);
      });
}

MonsterAIFactory MonsterAIFactory::wildlifeNonPredator() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new Fighter(c, 1.2, false),
          new Wildlife(c),
          new MoveRandomly(c)},
          {5, 6, 1});
      });
}

MonsterAIFactory MonsterAIFactory::moveRandomly() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new MoveRandomly(c)},
          {1});
      });
}

MonsterAIFactory MonsterAIFactory::stayOnFurniture(FurnitureType type) {
  return MonsterAIFactory([type](Creature* c) {
      return new MonsterAI(c, {
          new AvoidFire(c),
          new Fighter(c, 0.6, true),
          new StayOnFurniture(c, type)},
          {5, 2, 1});
      });
}

MonsterAIFactory MonsterAIFactory::idle() {
  return MonsterAIFactory([](Creature* c) {
      return new MonsterAI(c, {
          new Rest(c)},
          {1});
      });
}

MonsterAIFactory MonsterAIFactory::scavengerBird(Position corpsePos) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
          new BirdFlyAway(c, 3),
          new MoveRandomly(c),
          new GuardSquare(c, corpsePos, 1, 2)},
          {1, 1, 2});
      });
}

MonsterAIFactory MonsterAIFactory::guardSquare(Position pos) {
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
          new AvoidFire(c),
          new Heal(c),
          new Fighter(c, 0.6, true),
          new MoveRandomly(c),
          new GoldLust(c)},
          { 6, 5, 4, 3, 1, 1 });
      });
}

MonsterAIFactory MonsterAIFactory::dieTime(double dieTime) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
          new DieTime(c, dieTime),
          new AvoidFire(c),
          new Heal(c),
          new Fighter(c, 0.6, true),
          new MoveRandomly(c),
          new GoldLust(c)},
          { 6, 5, 4, 3, 1, 1 });
      });
}

MonsterAIFactory MonsterAIFactory::splashHeroes(bool leader) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
        leader ? (Behaviour*)new SplashHeroLeader(c) : (Behaviour*)new SplashHeroes(c),
        new Heal(c),
        new Fighter(c, 0.6, true),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c)}, {3, 1})},
        { 6, 5, 2, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::splashMonsters() {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
        new SplashMonsters(c),
        new Heal(c),
        new Fighter(c, 0.6, true),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c)}, {3, 1})},
        { 6, 5, 2, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::splashImps(const string& splashPath) {
  return MonsterAIFactory([=](Creature* c) {
      return new MonsterAI(c, {
        new SplashImps(c, splashPath),
        new Heal(c),
        new Fighter(c, 0.6, true),
        new ChooseRandom(c, {new Rest(c), new MoveRandomly(c)}, {3, 1})},
        { 6, 5, 2, 1}, false);
      });
}

