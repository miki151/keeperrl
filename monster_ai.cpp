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
#include "player_message.h"
#include "equipment.h"
#include "spell.h"
#include "creature_name.h"
#include "skill.h"
#include "task.h"
#include "game.h"
#include "creature_attributes.h"
#include "creature_factory.h"
#include "spell_map.h"
#include "effect.h"
#include "body.h"
#include "item_class.h"
#include "furniture.h"
#include "furniture_factory.h"
#include "file_path.h"
#include "ranged_weapon.h"
#include "task_map.h"
#include "collective_teams.h"
#include "collective_config.h"
#include "territory.h"
#include "minion_equipment.h"
#include "zones.h"
#include "team_order.h"

class Behaviour {
  public:
  Behaviour(WCreature);
  virtual MoveInfo getMove() { return NoMove; }
  virtual void onAttacked(WConstCreature attacker) {}
  virtual double itemValue(WConstItem) { return 0; }
  WItem getBestWeapon();
  WCreature getClosestEnemy();
  WCreature getClosestCreature();
  MoveInfo tryEffect(Effect, TimeInterval maxTurns = 1_visible);
  MoveInfo tryEffect(DirEffectType, Vec2);

  virtual ~Behaviour() {}

  SERIALIZE_ALL(creature)
  SERIALIZATION_CONSTRUCTOR(Behaviour)

  protected:
  WCreature SERIAL(creature);
};

MonsterAI::~MonsterAI() {}

SERIALIZE_DEF(MonsterAI, behaviours, weights, creature, pickItems)
SERIALIZATION_CONSTRUCTOR_IMPL(MonsterAI);

Behaviour::Behaviour(WCreature c) : creature(c) {
}

WCreature Behaviour::getClosestEnemy() {
  PROFILE;
  int dist = 1000000000;
  WCreature result = nullptr;
  for (WCreature other : creature->getVisibleEnemies()) {
    int curDist = other->getPosition().dist8(creature->getPosition());
    if (curDist < dist &&
        (!other->getAttributes().dontChase() || curDist == 1) &&
        !other->isAffected(LastingEffect::STUNNED)) {
      result = other;
      dist = creature->getPosition().dist8(other->getPosition());
    }
  }
  return result;
}

WCreature Behaviour::getClosestCreature() {
  int dist = 1000000000;
  WCreature result = nullptr;
  for (WCreature other : creature->getVisibleCreatures())
    if (other != creature && other->getPosition().dist8(creature->getPosition()) < dist) {
      result = other;
      dist = creature->getPosition().dist8(other->getPosition());
    }
  return result;
}

WItem Behaviour::getBestWeapon() {
  WItem best = nullptr;
  int damage = -1;
  for (WItem item : creature->getEquipment().getItems().filter(Item::classPredicate(ItemClass::WEAPON)))
    if (item->getModifier(AttrType::DAMAGE) > damage) {
      damage = item->getModifier(AttrType::DAMAGE);
      best = item;
    }
  return best;
}

MoveInfo Behaviour::tryEffect(Effect type, TimeInterval maxTurns) {
  if (auto effect = type.getValueMaybe<Effect::Lasting>())
    if (creature->isAffected(effect->lastingEffect))
      return NoMove;
  for (Spell* spell : creature->getAttributes().getSpellMap().getAll()) {
   if (spell->hasEffect(type))
      if (auto action = creature->castSpell(spell))
        return { 1, action };
  }
  auto items = creature->getEquipment().getItems().filter(Item::effectPredicate(type));
  for (WItem item : items)
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
  Heal(WCreature c) : Behaviour(c) {}

  virtual double itemValue(WConstItem item) {
    if (auto& effect = item->getEffect())
      if (effect->isType<Effect::Heal>())
        return 0.5;
    return 0;
  }

  MoveInfo tryHealingOther() {
    if (creature->getAttributes().getSpellMap().contains(SpellId::HEAL_OTHER)) {
      MoveInfo healAction = NoMove;
      for (Vec2 v : Vec2::directions8(Random))
        if (WConstCreature other = creature->getPosition().plus(v).getCreature())
          if (creature->isFriend(other) && other->getBody().canHeal())
            if (auto action = creature->castSpell(Spell::get(SpellId::HEAL_OTHER), v)) {
              healAction = MoveInfo(0.5, action);
              // Prioritize the action if there is an enemy next to the healed creature.
              for (auto pos : other->getPosition().neighbors8())
                if (auto enemy = pos.getCreature())
                  if (other->isEnemy(enemy))
                    return healAction;
            }
      if (healAction)
        return healAction;
    }
    return NoMove;
  }

  virtual MoveInfo getMove() {
    if (auto move = tryHealingOther())
      return move;
    if (!creature->getBody().isHumanoid())
      return NoMove;
    if (creature->isAffected(LastingEffect::POISON)) {
      if (MoveInfo move = tryEffect(Effect::Lasting{LastingEffect::POISON_RESISTANT}))
        return move;
      if (MoveInfo move = tryEffect(Effect::CurePoison{}))
        return move;
    }
    if (creature->getBody().canHeal()) {
      if (MoveInfo move = tryEffect(Effect::Heal{}))
        return move.withValue(min(1.0, 1.5 - creature->getBody().getHealth()));
      if (MoveInfo move = tryEffect(Effect::Lasting{LastingEffect::REGENERATION}))
        return move;
      if (MoveInfo move = tryEffect(Effect::Heal{}, 3_visible))
        return move.withValue(0.5 * min(1.0, 1.5 - creature->getBody().getHealth()));
    }
    for (Position pos : creature->getPosition().neighbors8())
      if (WCreature c = pos.getCreature())
        if (creature->isFriend(c) && c->getBody().canHeal())
          if (c->getEquipment().getItems(ItemIndex::HEALING_ITEM).empty())
            for (WItem item : creature->getEquipment().getItems(ItemIndex::HEALING_ITEM))
              if (auto action = creature->give(c, {item}))
                return MoveInfo(0.5, action);
    return NoMove;
  }


  SERIALIZE_ALL(SUBCLASS(Behaviour));
  SERIALIZATION_CONSTRUCTOR(Heal);
};

class Rest : public Behaviour {
  public:
  Rest(WCreature c) : Behaviour(c) {}

  virtual MoveInfo getMove() {
    return {0.1, creature->wait() };
  }

  SERIALIZATION_CONSTRUCTOR(Rest);
  SERIALIZE_ALL(SUBCLASS(Behaviour));
};

class MoveRandomly : public Behaviour {
  public:
  MoveRandomly(WCreature c) : Behaviour(c) {}

  virtual MoveInfo getMove() {
    if (!visited(creature->getPosition()))
      updateMem(creature->getPosition());
    optional<Position> target;
    double val = 0.0001;
    if (Random.roll(2))
      return {val, creature->wait()};
    for (Position pos : creature->getPosition().neighbors8(Random)) {
      if (Random.roll(10))
        if (auto other = pos.getCreature())
          if (auto petAction = creature->pet(other))
            return petAction;
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
      return {val, creature->move(*target).append([=] (WCreature creature) {
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
  SERIALIZE_ALL(SUBCLASS(Behaviour));

  private:
  deque<Position> memory;
  const int memSize = 3;
};

class StayOnFurniture : public Behaviour {
  public:
  StayOnFurniture(WCreature c, FurnitureType t) : Behaviour(c), type(t) {}

  MoveInfo getMove() override {
    if (!creature->getPosition().getFurniture(type)) {
      if (!nextPigsty)
        for (auto pos : creature->getPosition().getRectangle(Rectangle::centered(Vec2(0, 0), 20)))
          if (pos.getFurniture(type)) {
            nextPigsty = pos;
            break;
          }
      if (nextPigsty)
        if (auto move = creature->moveTowards(*nextPigsty, Creature::NavigationFlags().requireStepOnTile()))
          return move;
    }
    if (Random.roll(10))
      for (Position next: creature->getPosition().neighbors8(Random))
        if (next.canEnter(creature) && next.getFurniture(type))
          return creature->move(next);
    return creature->wait();
  }

  SERIALIZATION_CONSTRUCTOR(StayOnFurniture)
  SERIALIZE_ALL(SUBCLASS(Behaviour), type)

  private:
  FurnitureType SERIAL(type);
  optional<Position> nextPigsty;
};

class BirdFlyAway : public Behaviour {
  public:
  BirdFlyAway(WCreature c, double _maxDist) : Behaviour(c), maxDist(_maxDist) {}

  virtual MoveInfo getMove() override {
    WConstCreature enemy = getClosestEnemy();
    if (Random.roll(15) || ( enemy && enemy->getPosition().dist8(creature->getPosition()) < maxDist))
      if (auto action = creature->flyAway())
        return {1.0, action};
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(BirdFlyAway)
  SERIALIZE_ALL(SUBCLASS(Behaviour), maxDist)

  private:
  double SERIAL(maxDist);
};

class GoldLust : public Behaviour {
  public:
  GoldLust(WCreature c) : Behaviour(c) {}

  virtual double itemValue(WConstItem item) {
    if (item->getClass() == ItemClass::GOLD)
      return 1;
    else
      return 0;
  }

  SERIALIZATION_CONSTRUCTOR(GoldLust)
  SERIALIZE_ALL(SUBCLASS(Behaviour))
};

class Wildlife : public Behaviour {
  public:
  Wildlife(WCreature c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    if (WCreature other = getClosestCreature()) {
      int dist = creature->getPosition().dist8(other->getPosition());
      if (dist == 1)
        return creature->attack(other);
      if (dist < 7)
        // pathfinding is expensive so only do it when running away from the player
        return creature->moveAway(other->getPosition(), other->isPlayer());
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(Wildlife)
  SERIALIZE_ALL(SUBCLASS(Behaviour))
};

class Fighter : public Behaviour {
  public:
  Fighter(WCreature c, double powerR) : Behaviour(c), maxPowerRatio(powerR) {
  }

  virtual MoveInfo getMove() override {
    return getMove(true);
  }

  MoveInfo getMove(bool chase) {
    if (WCreature other = getClosestEnemy()) {
      double myDamage = creature->getDefaultWeaponDamage();
      double powerRatio = myDamage / (other->getDefaultWeaponDamage() + 1);
      bool significantEnemy = myDamage < 5 * other->getDefaultWeaponDamage();
      double panicWeight = 0;
      if (powerRatio < maxPowerRatio)
        panicWeight += 2 - powerRatio * 2;
      panicWeight -= creature->getAttributes().getCourage();
      panicWeight -= creature->getMorale() * 0.3;
      panicWeight = min(1.0, max(0.0, panicWeight));
      if (creature->isAffected(LastingEffect::PANIC) || creature->getStatus().contains(CreatureStatus::CIVILIAN))
        panicWeight = 1;
      if (other->hasCondition(CreatureCondition::SLEEPING))
        panicWeight = 0;
      INFO << creature->getName().bare() << " panic weight " << panicWeight;
      if (panicWeight >= 0.5) {
        double dist = creature->getPosition().dist8(other->getPosition());
        if (dist < 7) {
          if (dist > 3)
            if (auto move = getFireMove(creature->getPosition().getDir(other->getPosition()), other))
              return move;
          if (chase)
            if (MoveInfo move = getPanicMove(other, panicWeight))
              return move;
          return getAttackMove(other, significantEnemy && chase);
        }
        return NoMove;
      } else
        return getAttackMove(other, significantEnemy && chase);
    } else if (chase)
      return getLastSeenMove();
    else
      return NoMove;
  }

  void setLastCombatIntent(WCreature attacked) {
    if (attacked->canSee(creature))
      attacked->setLastCombatIntent({creature->getName().a(), *creature->getGlobalTime()});
    creature->setLastCombatIntent({attacked->getName().a(), *creature->getGlobalTime()});
  }

  MoveInfo getPanicMove(WCreature other, double weight) {
    if (auto teleMove = tryEffect(Effect::Teleport{}))
      return teleMove.withValue(weight);
    if (auto action = creature->moveAway(other->getPosition(), true))
      return {weight, action.prepend([=](WCreature creature) {
        lastSeen = LastSeen{creature->getPosition(), *creature->getGlobalTime(), LastSeen::PANIC, other->getUniqueId()};
      })};
    else
      return NoMove;
  }

  virtual double itemValue(WConstItem item) override {
    if (auto& effect = item->getEffect())
      if (contains<Effect>({
            Effect::Lasting{LastingEffect::INVISIBLE},
            Effect::Lasting{LastingEffect::SLOWED},
            Effect::Lasting{LastingEffect::BLIND},
            Effect::Lasting{LastingEffect::SLEEP},
            Effect::Lasting{LastingEffect::POISON},
            Effect::Lasting{LastingEffect::POISON_RESISTANT},
            Effect::CurePoison{},
            Effect::Teleport{},
            Effect::Lasting{LastingEffect::DAM_BONUS},
            Effect::Lasting{LastingEffect::DEF_BONUS}},
            *effect))
      return 1;
    int damage = item->getModifier(AttrType::DAMAGE);
    WItem best = getBestWeapon();
    if (best && best != item && best->getModifier(AttrType::DAMAGE) >= damage)
        return 0;
    return (double)damage / 50;
  }

  bool checkFriendlyFire(Vec2 enemyDir) {
    Vec2 dir = enemyDir.shorten();
    for (Vec2 v = dir; v != enemyDir; v += dir) {
      WConstCreature c = creature->getPosition().plus(v).getCreature();
      if (c && !creature->isEnemy(c))
        return true;
    }
    return false;
  }

  int getThrowValue(WItem it) {
    if (auto& effect = it->getEffect())
      if (contains<Effect>({
            Effect::Lasting{LastingEffect::POISON},
            Effect::Lasting{LastingEffect::SLOWED},
            Effect::Lasting{LastingEffect::BLIND},
            Effect::Lasting{LastingEffect::SLEEP}},
            *effect))
        return 100;
    return 0;
  }

  MoveInfo getThrowMove(Vec2 enemyDir, WCreature other) {
    if (enemyDir.x != 0 && enemyDir.y != 0 && abs(enemyDir.x) != abs(enemyDir.y))
      return NoMove;
    if (checkFriendlyFire(enemyDir))
      return NoMove;
    Vec2 dir = enemyDir.shorten();
    WItem best = nullptr;
    int damage = 0;
    for (WItem item : creature->getEquipment().getItems())
      if (!creature->getEquipment().isEquipped(item) && getThrowValue(item) > damage) {
        damage = getThrowValue(item);
        best = item;
      }
    if (best)
      if (auto action = creature->throwItem(best, dir))
        return {1.0, action.append([=](WCreature) { setLastCombatIntent(other); }) };
    return NoMove;
  }

  vector<DirEffectType> getOffensiveEffects() {
    static vector<DirEffectType> effects = [] {
      vector<DirEffectType> ret;
      for (auto id : {SpellId::BLAST, SpellId::MAGIC_MISSILE})
        ret.push_back(Spell::get(id)->getDirEffectType());
      return ret;
    }();
    return effects;
  }

  MoveInfo getFireMove(Vec2 dir, WCreature other) {
    if (dir.x != 0 && dir.y != 0 && abs(dir.x) != abs(dir.y))
      return NoMove;
    if (checkFriendlyFire(dir))
      return NoMove;
    for (auto effect : getOffensiveEffects())
      if (auto action = tryEffect(effect, dir.shorten()))
        return action;
    if (auto action = creature->fire(dir.shorten()))
      return {1.0, action.append([=](WCreature creature) {
          setLastCombatIntent(other);
      })};
    return NoMove;
  }

  MoveInfo getLastSeenMove() {
    if (auto lastSeen = getLastSeen()) {
      auto lastSeenTimeout = 20_visible;
      if (!lastSeen->pos.isSameLevel(creature->getPosition()) ||
          lastSeen->time < *creature->getGlobalTime() - lastSeenTimeout ||
          lastSeen->pos == creature->getPosition()) {
        lastSeen = none;
        return NoMove;
      }
      if (lastSeen->type == LastSeen::ATTACK)
        if (auto action = creature->moveTowards(lastSeen->pos)) {
          return {0.5, action};
        }
      if (lastSeen->type == LastSeen::PANIC && lastSeen->pos.dist8(creature->getPosition()) < 4)
        if (auto action = creature->moveAway(lastSeen->pos, true))
          return {0.5, action};
    }
    return NoMove;
  }

  MoveInfo considerCircularBlast() {
    int numEnemies = 0;
    auto pos = creature->getPosition();
    for (Vec2 v : Vec2::directions8())
      if (auto c = pos.plus(v).getCreature())
        if (c->isEnemy(creature))
          ++numEnemies;
    if (numEnemies >= 3)
      if (MoveInfo move = tryEffect(Effect::CircularBlast{}))
        return move;
    return NoMove;
  }

  MoveInfo considerEquippingWeapon(WCreature other, int distance) {
    if (creature->getBody().isHumanoid() && !creature->getWeapon()) {
      if (WItem weapon = getBestWeapon())
        if (auto action = creature->equip(weapon))
          return {3.0 / (2.0 + distance), action.prepend([=](WCreature creature) {
            setLastCombatIntent(other);
        })};
    }
    return NoMove;
  }

  MoveInfo considerBuffs() {
    for (Effect effect : {
        Effect(Effect::Lasting{LastingEffect::INVISIBLE}),
        Effect(Effect::Lasting{LastingEffect::DAM_BONUS}),
        Effect(Effect::Lasting{LastingEffect::DEF_BONUS}),
        Effect(Effect::Lasting{LastingEffect::SPEED}),
        Effect(Effect::Deception{}),
        Effect(Effect::Summon{CreatureId::SPIRIT})})
      if (MoveInfo move = tryEffect(effect))
        return move;
    return NoMove;
  }

  MoveInfo considerBreakingChokePoint(WCreature other) {
  PROFILE;
    unordered_set<Position, CustomHash<Position>> myNeighbors;
    for (auto pos : creature->getPosition().neighbors8(Random))
      myNeighbors.insert(pos);
    MoveInfo destroyMove = NoMove;
    bool isFriendBetween = false;
    for (auto pos : other->getPosition().neighbors8())
      if (myNeighbors.count(pos)) {
        if (pos.canEnter(creature))
          return NoMove;
        if (auto c = pos.getCreature())
          if (c->isFriend(creature)) {
            isFriendBetween = true;
            continue;
          }
        /*if (auto move = creature->move(pos))
          return move;*/
        if (!destroyMove)
          if (auto destroyAction = pos.getBestDestroyAction(creature->getMovementType()))
            if (auto move = creature->destroy(creature->getPosition().getDir(pos), *destroyAction))
              destroyMove = move;
      }
    if (isFriendBetween) {
      if (auto move = tryEffect(Effect::DestroyWalls{}))
        return move;
      return destroyMove;
    } else
      return NoMove;
  }

  int getFiringRange(WConstCreature c) {
    auto weapon = c->getEquipment().getSlotItems(EquipmentSlot::RANGED_WEAPON);
    if (weapon.empty())
      return 0;
    if (c->getAmmoHeld() < 1) return 0;
    return weapon.getOnlyElement()->getRangedWeapon()->getMaxDistance();
  }

  MoveInfo getAttackMove(WCreature other, bool chase) {
    CHECK(other);
    if (other->getAttributes().isBoulder())
      return NoMove;
    INFO << creature->getName().bare() << " enemy " << other->getName().bare();
    Vec2 enemyDir = creature->getPosition().getDir(other->getPosition());
    auto distance = enemyDir.length8();
    if (auto move = considerEquippingWeapon(other, distance))
      return move;
    if (distance == 1)
      if (auto move = considerCircularBlast())
        return move;
    if (distance <= 5)
      if (auto move = considerBuffs())
        return move;
    if (distance > 1 && distance <= getFiringRange(creature))
        if (MoveInfo move = getFireMove(enemyDir, other))
          return move;
    if (distance > 1 && distance <= 10)
        if (MoveInfo move = getThrowMove(enemyDir, other))
          return move;
    if (distance > 1) {
      if (chase && !other->getAttributes().dontChase() && !isChaseFrozen(other)) {
        lastSeen = none;
        if (auto action = creature->moveTowards(other->getPosition()))
          return {max(0., 1.0 - double(distance) / 20), action.prepend([=](WCreature creature) {
            setLastCombatIntent(other);
            lastSeen = LastSeen{other->getPosition(), *creature->getGlobalTime(), LastSeen::ATTACK, other->getUniqueId()};
            auto chaseInfo = chaseFreeze.getMaybe(other);
            auto startChaseFreeze = 20_visible;
            auto endChaseFreeze = 20_visible;
            auto time = *other->getGlobalTime();
            if (!chaseInfo || time > chaseInfo->second)
              chaseFreeze.set(other, make_pair(time + startChaseFreeze, time + endChaseFreeze));
          })};
      }
      if (distance == 2)
        if (auto move = considerBreakingChokePoint(other))
          return move;
    }
    if (distance == 1)
      if (auto action = creature->attack(other, getAttackParams(other)))
        return {1.0, action.prepend([=](WCreature) {
            setLastCombatIntent(other);
        })};
    return NoMove;
  }

  Creature::AttackParams getAttackParams(WConstCreature) {
    return Creature::AttackParams {};
  }

  SERIALIZATION_CONSTRUCTOR(Fighter)
  SERIALIZE_ALL(SUBCLASS(Behaviour), maxPowerRatio)

  private:
  double SERIAL(maxPowerRatio);
  struct LastSeen {
    Position SERIAL(pos);
    GlobalTime SERIAL(time);
    enum { ATTACK, PANIC} SERIAL(type);
    Creature::Id SERIAL(creature);
    SERIALIZE_ALL(pos, time, type, creature)
  };
  optional<LastSeen> lastSeen;
  optional<LastSeen>& getLastSeen() {
    if (lastSeen && !creature->getLevel()->containsCreature(lastSeen->creature))
      lastSeen.reset();
    return lastSeen;
  }
  EntityMap<Creature, pair<GlobalTime, GlobalTime>> chaseFreeze;

  bool isChaseFrozen(WConstCreature c) {
    auto chaseInfo = chaseFreeze.getMaybe(c);
    auto time = *c->getGlobalTime();
    return chaseInfo && chaseInfo->first <= time && chaseInfo->second >= time;
  }
};

class FighterStandGround : public Behaviour {
  public:
  FighterStandGround(WCreature c, double powerR) : Behaviour(c), fighter(unique<Fighter>(c, powerR)) {
  }

  virtual MoveInfo getMove() override {
    return fighter->getMove(false);
  }

  SERIALIZATION_CONSTRUCTOR(FighterStandGround)
  SERIALIZE_ALL(SUBCLASS(Behaviour), fighter)

  private:
  unique_ptr<Fighter> SERIAL(fighter);
};

class GuardTarget : public Behaviour {
  public:
  GuardTarget(WCreature c, double minD, double maxD) : Behaviour(c), minDist(minD), maxDist(maxD) {}

  SERIALIZATION_CONSTRUCTOR(GuardTarget)
  SERIALIZE_ALL(SUBCLASS(Behaviour), minDist, maxDist)

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
  GuardArea(WCreature c, Rectangle a) : Behaviour(c), area(a) {}

  virtual MoveInfo getMove() override {
    if (!myLevel)
      myLevel = creature->getLevel();
    if (auto action = creature->stayIn(myLevel, area))
      return {1.0, action};
    else
      return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(GuardArea);
  SERIALIZE_ALL(SUBCLASS(Behaviour), myLevel, area);

  private:
  WLevel SERIAL(myLevel) = nullptr;
  Rectangle SERIAL(area);
};

class GuardSquare : public GuardTarget {
  public:
  GuardSquare(WCreature c, Position _pos, double minDist, double maxDist) : GuardTarget(c, minDist, maxDist),
      pos(_pos) {}

  virtual MoveInfo getMove() override {
    return getMoveTowards(pos);
  }

  SERIALIZATION_CONSTRUCTOR(GuardSquare);
  SERIALIZE_ALL(SUBCLASS(GuardTarget), pos);

  private:
  Position SERIAL(pos);
};

class Wait : public Behaviour {
  public:
  Wait(WCreature c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    return {1.0, creature->wait()};
  }

  SERIALIZATION_CONSTRUCTOR(Wait);
  SERIALIZE_ALL(SUBCLASS(Behaviour));
};

class DieTime : public Behaviour {
  public:
  DieTime(WCreature c, GlobalTime t) : Behaviour(c), dieTime(t) {}

  virtual MoveInfo getMove() override {
    if (creature->getGlobalTime() > dieTime) {
      return {1.0, CreatureAction(creature, [=](WCreature creature) {
        creature->dieNoReason(Creature::DropType::NOTHING);
      })};
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(DieTime);
  SERIALIZE_ALL(SUBCLASS(Behaviour), dieTime);

  private:
  GlobalTime SERIAL(dieTime);
};

class Summoned : public GuardTarget {
  public:
  Summoned(WCreature c, WCreature _target, double minDist, double maxDist)
      : GuardTarget(c, minDist, maxDist), target(_target) {
  }

  virtual MoveInfo getMove() override {
    if (target->isDead()) {
      return {1.0, CreatureAction(creature, [=](WCreature creature) {
        creature->dieNoReason(Creature::DropType::NOTHING);
      })};
    }
    if (MoveInfo move = getMoveTowards(target->getPosition()))
      return move.withValue(0.5);
    else
      return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(Summoned);
  SERIALIZE_ALL(SUBCLASS(GuardTarget), target, dieTime);

  private:
  WCreature SERIAL(target);
  GlobalTime SERIAL(dieTime);
};

class Thief : public Behaviour {
  public:
  Thief(WCreature c) : Behaviour(c) {}
 
  virtual MoveInfo getMove() override {
    if (!creature->getAttributes().getSkills().hasDiscrete(SkillId::STEALING))
      return NoMove;
    for (WConstCreature other : creature->getVisibleEnemies()) {
      if (robbed.contains(other)) {
        if (MoveInfo teleMove = tryEffect(Effect::Teleport{}))
          return teleMove;
        if (auto action = creature->moveAway(other->getPosition()))
        return {1.0, action};
      }
    }
    for (Position pos : creature->getPosition().neighbors8(Random)) {
      WConstCreature other = pos.getCreature();
      if (other && !robbed.contains(other)) {
        vector<WItem> allGold;
        for (WItem it : other->getEquipment().getItems())
          if (it->getClass() == ItemClass::GOLD)
            allGold.push_back(it);
        if (allGold.size() > 0)
          if (auto action = creature->stealFrom(creature->getPosition().getDir(other->getPosition()), allGold))
          return {1.0, action.append([=](WCreature creature) {
            other->secondPerson(creature->getName().the() + " steals all your gold!");
            robbed.insert(other);
          })};
      }
    }
    return NoMove;
  }

  SERIALIZATION_CONSTRUCTOR(Thief);
  SERIALIZE_ALL(SUBCLASS(Behaviour), robbed);

  private:
  EntitySet<Creature> SERIAL(robbed);
};

class ByCollective : public Behaviour {
  public:
  ByCollective(WCreature c, WCollective col, unique_ptr<Fighter> fighter) : Behaviour(c), collective(col),
      fighter(std::move(fighter)) {}

  MoveInfo priorityTask() {
    auto& taskMap = collective->getTaskMap();
    if (auto task = taskMap.getTask(creature))
      if (taskMap.isPriorityTask(task))
        return task->getMove(creature).orWait();
    for (auto activity : ENUM_ALL(MinionActivity))
      if (creature->getAttributes().getMinionActivities().isAvailable(collective, creature, activity))
        if (auto task = taskMap.getClosestTask(creature, activity, true)) {
          taskMap.freeFromTask(creature);
          taskMap.takeTask(creature, task);
          return task->getMove(creature).orWait();
        }
    return NoMove;
  };

  optional<TeamId> getActiveTeam() {
    auto& teams = collective->getTeams();
    for (auto team : teams.getContaining(creature))
      if (teams.isActive(team))
        return team;
    return none;
  }

  MoveInfo getFighterMove() {
    auto& teams = collective->getTeams();
    if (auto currentTeam = getActiveTeam())
      if (teams.hasTeamOrder(*currentTeam, creature, TeamOrder::FLEE) ||
          teams.hasTeamOrder(*currentTeam, creature, TeamOrder::STAND_GROUND)) {
        return fighter->getMove(false);
      }
    return fighter->getMove(true);
  }

  MoveInfo followTeamLeader() {
    auto& teams = collective->getTeams();
    if (auto team = getActiveTeam()) {
      if (!teams.hasTeamOrder(*team, creature, TeamOrder::STAND_GROUND)) {
        WConstCreature leader = teams.getLeader(*team);
        if (creature != leader) {
          if (leader->getPosition().dist8(creature->getPosition()) > 1)
            return creature->moveTowards(leader->getPosition());
          else
            return creature->wait();
        } else
          if (creature == leader && !teams.isPersistent(*team))
            return creature->wait();
      } else
        return creature->wait();
    }
    return NoMove;
  };

  MoveInfo dropLoot() {
    auto& config = collective->getConfig();
    if (config.getFetchItems() && collective->getTerritory().contains(creature->getPosition())) {
      vector<WItem> items = creature->getEquipment().getItems().filter([this](WConstItem item) {
          return !collective->isItemMarked(item) && !collective->getMinionEquipment().isOwner(item, creature); });
      if (!items.empty())
        return creature->drop(items);
    }
    return NoMove;
  };

  PTask getEquipmentTask() {
    if (!collective->usesEquipment(creature))
      return nullptr;
    auto& minionEquipment = collective->getMinionEquipment();
    if (!collective->hasTrait(creature, MinionTrait::NO_AUTO_EQUIPMENT) && Random.roll(40))
      minionEquipment.autoAssign(creature, collective->getAllItems(ItemIndex::MINION_EQUIPMENT, false));
    vector<PTask> tasks;
    for (WItem it : creature->getEquipment().getItems())
      if (!creature->getEquipment().isEquipped(it) && creature->getEquipment().canEquip(it))
        tasks.push_back(Task::equipItem(it));
    for (Position v : collective->getZones().getPositions(ZoneId::STORAGE_EQUIPMENT)) {
      vector<WItem> allItems = v.getItems(ItemIndex::MINION_EQUIPMENT).filter(
          [&minionEquipment, this] (WConstItem it) { return minionEquipment.isOwner(it, creature);});
      vector<WItem> consumables;
      for (auto item : allItems)
        if (item->canEquip())
          tasks.push_back(Task::pickAndEquipItem(collective, v, item));
        else
          consumables.push_back(item);
      if (!consumables.empty())
        tasks.push_back(Task::pickItem(collective, v, consumables));
    }
    if (!tasks.empty())
      return Task::chain(std::move(tasks));
    return nullptr;
  }

  void setRandomTask() {
    vector<MinionActivity> goodTasks;
    for (MinionActivity t : ENUM_ALL(MinionActivity))
      if (collective->isActivityGood(creature, t) && creature->getAttributes().getMinionActivities().canChooseRandomly(creature, t))
        goodTasks.push_back(t);
    if (!goodTasks.empty())
      collective->setMinionActivity(creature, Random.choose(goodTasks));
  }

  WTask getStandardTask() {
    PROFILE;
    auto current = collective->getCurrentActivity(creature);
    if (current.task == MinionActivity::IDLE || !collective->isActivityGood(creature, current.task)) {
      collective->setMinionActivity(creature, MinionActivity::IDLE);
      setRandomTask();
    }
    current = collective->getCurrentActivity(creature);
    MinionActivity task = current.task;
    auto& taskMap = collective->getTaskMap();
    if (current.finishTime < collective->getLocalTime())
      collective->setMinionActivity(creature, MinionActivity::IDLE);
    if (PTask ret = MinionActivities::generate(collective, creature, task))
      return taskMap.addTaskFor(std::move(ret), creature);
    if (WTask ret = MinionActivities::getExisting(collective, creature, task)) {
      taskMap.takeTask(creature, ret);
      return ret;
    }
    FATAL << "No task generated for activity " << EnumInfo<MinionActivity>::getString(task);
    return {};
  }

  MoveInfo goToAlarm() {
    auto& alarmInfo = collective->getAlarmInfo();
    if (collective->hasTrait(creature, MinionTrait::FIGHTER) && alarmInfo && alarmInfo->finishTime > collective->getGlobalTime())
      if (auto action = creature->moveTowards(alarmInfo->position))
        return {1.0, action};
    return NoMove;
  };

  MoveInfo normalTask() {
    if (WTask task = collective->getTaskMap().getTask(creature))
      return task->getMove(creature).orWait();
    return NoMove;
  };

  MoveInfo newEquipmentTask() {
    if (PTask t = getEquipmentTask())
      if (auto move = t->getMove(creature)) {
        collective->getTaskMap().addTaskFor(std::move(t), creature);
        return move;
      }
    return NoMove;
  };

  MoveInfo newStandardTask() {
    WTask t = getStandardTask();
    return t->getMove(creature).orWait();
  };

  void considerHealingTask() {
    const static EnumSet<MinionActivity> healingTasks {MinionActivity::SLEEP};
    PROFILE;
    if (creature->getBody().canHeal() && !creature->isAffected(LastingEffect::POISON))
      for (MinionActivity t : healingTasks) {
        auto currentTask = collective->getCurrentActivity(creature).task;
        if (creature->getAttributes().getMinionActivities().isAvailable(collective, creature, t) &&
            !healingTasks.contains(currentTask)) {
          collective->cancelTask(creature);
          collective->setMinionActivity(creature, t);
          return;
        }
      }
  }

  static MoveInfo getFirstGoodMove() {
    return NoMove;
  }

  template <typename MoveFun1, typename... MoveFuns>
  static MoveInfo getFirstGoodMove(MoveFun1&& f1, MoveFuns&&... funs) {
    if (auto move = std::forward<MoveFun1>(f1)())
      return move;
    else
      return getFirstGoodMove(std::forward<MoveFuns>(funs)...);
  }

  virtual MoveInfo getMove() override {
    if (collective->getConfig().allowHealingTaskOutsideTerritory() || collective->getTerritory().contains(creature->getPosition()))
      considerHealingTask();
    return getFirstGoodMove(
        bindMethod(&ByCollective::getFighterMove, this),
        bindMethod(&ByCollective::priorityTask, this),
        bindMethod(&ByCollective::followTeamLeader, this),
        bindMethod(&ByCollective::dropLoot, this),
        bindMethod(&ByCollective::goToAlarm, this),
        bindMethod(&ByCollective::normalTask, this),
        bindMethod(&ByCollective::newEquipmentTask, this),
        bindMethod(&ByCollective::newStandardTask, this)
    );
  }

  SERIALIZATION_CONSTRUCTOR(ByCollective);
  SERIALIZE_ALL(SUBCLASS(Behaviour), collective, fighter);

  private:
  WCollective SERIAL(collective);
  unique_ptr<Fighter> SERIAL(fighter);
};

class ChooseRandom : public Behaviour {
  public:
  ChooseRandom(WCreature c, vector<PBehaviour>&& beh, vector<double> w)
      : Behaviour(c), behaviours(std::move(beh)), weights(w) {}

  virtual MoveInfo getMove() override {
    return behaviours[Random.get(weights)]->getMove();
  }

  SERIALIZATION_CONSTRUCTOR(ChooseRandom);
  SERIALIZE_ALL(SUBCLASS(Behaviour), behaviours, weights);

  private:
  vector<PBehaviour> SERIAL(behaviours);
  vector<double> SERIAL(weights);
};

class SingleTask : public Behaviour {
  public:
  SingleTask(WCreature c, PTask t) : Behaviour(c), task(std::move(t)) {}

  virtual MoveInfo getMove() override {
    return task->getMove(creature);
  };

  SERIALIZATION_CONSTRUCTOR(SingleTask);
  SERIALIZE_ALL(SUBCLASS(Behaviour), task);

  private:
  PTask SERIAL(task);
};

const static Vec2 splashTarget = Level::getSplashBounds().middle() - Vec2(3, 0);
const static Vec2 splashLeaderPos = Vec2(Level::getSplashVisibleBounds().right(),
    Level::getSplashBounds().middle().y) - Vec2(4, 0);

class SplashHeroes : public Behaviour {
  public:
  SplashHeroes(WCreature c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->getAttributes().setCourage(1);
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
  SERIALIZE_ALL(SUBCLASS(Behaviour));

  private:
  bool started = false;
};

class SplashHeroLeader : public Behaviour {
  public:
  SplashHeroLeader(WCreature c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->getAttributes().setCourage(1);
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
  SERIALIZE_ALL(SUBCLASS(Behaviour));

  private:

  bool started = false;
};

class SplashMonsters : public Behaviour {
  public:
  SplashMonsters(WCreature c) : Behaviour(c) {}

  virtual MoveInfo getMove() override {
    creature->getAttributes().setCourage(1);
    if (!initialPos)
      initialPos = creature->getPosition().getCoord();
    vector<WCreature> heroes;
    for (auto c : creature->getLevel()->getAllCreatures())
      if (c->isEnemy(creature))
        heroes.push_back(c);
    if (heroes.empty()) {
      if (creature->getPosition().getCoord() == *initialPos)
        return creature->wait();
      else
        return creature->moveTowards(Position(*initialPos, creature->getLevel()));
    }
    if (WConstCreature other = Position(splashTarget, creature->getLevel()).getCreature())
      if (creature->isEnemy(other))
        attack = true;
    if (!attack)
      return creature->wait();
    else
      return {0.1, creature->moveTowards(Random.choose(heroes)->getPosition())};
  };

  SERIALIZATION_CONSTRUCTOR(SplashMonsters);
  SERIALIZE_ALL(SUBCLASS(Behaviour));

  private:
  optional<Vec2> initialPos;
  bool attack = false;
};

class SplashItems {
  public:
  void addItems(Vec2 pos, vector<WItem> v) {
    items[pos] = v;
  }

  Vec2 chooseClosest(Vec2 pos) {
    Vec2 ret(1000, 1000);
    for (Vec2 v : getKeys(items))
      if (v.dist8(pos) < ret.dist8(pos))
        ret = v;
    return ret;
  }

  OwnerPointer<TaskCallback> callbackDummy = makeOwner<TaskCallback>();

  PTask getNextTask(Vec2 position, WLevel level) {
    if (items.empty())
      return nullptr;
    Vec2 pos = chooseClosest(position);
    vector<WItem> it = {Random.choose(items[pos])};
    if (it[0]->getClass() == ItemClass::GOLD)
      for (WItem it2 : copyOf(items[pos]))
        if (it[0] != it2 && it2->getClass() == it[0]->getClass() && Random.roll(10))
          it.push_back(it2);
    for (WItem it2 : it)
      items[pos].removeElement(it2);
    if (items[pos].empty())
      items.erase(pos);
    vector<Vec2>& targets = it[0]->getClass() == ItemClass::GOLD ? targetsGold : targetsCorpse;
    if (targets.empty())
      return nullptr;
    Vec2 target = Random.choose(targets);
    targets.removeElement(target);
    return Task::bringItem(callbackDummy.get(), Position(pos, level), it, {Position(target, level)}, 100);
  }

  void setInitialized(const FilePath& splashPath) {
    initialized = true;
    ifstream iff(splashPath.getPath());
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
  map<Vec2, vector<WItem>> items;
  bool initialized = false;
  vector<Vec2> targetsGold;
  vector<Vec2> targetsCorpse;
};

static SplashItems splashItems;

class SplashImps : public Behaviour {
  public:
  SplashImps(WCreature c, const FilePath& splash) : Behaviour(c), splashPath(splash) {}

  void initializeSplashItems() {
    for (Vec2 v : Level::getSplashVisibleBounds()) {
      vector<WItem> inv = Position(v, creature->getLevel()).getItems().filter(
          [](WConstItem it) { return it->getClass() == ItemClass::GOLD || it->getClass() == ItemClass::CORPSE;});
      if (!inv.empty())
        splashItems.addItems(v, inv);
    }
    splashItems.setInitialized(*splashPath);
  }

  virtual MoveInfo getMove() override {
    creature->addEffect(LastingEffect::SPEED, 1000_visible);
    if (!initialPos)
      initialPos = creature->getPosition().getCoord();
    bool heroesDead = true;
    for (WConstCreature c : creature->getLevel()->getAllCreatures())
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
          task.clear();
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
  SERIALIZE_ALL(SUBCLASS(Behaviour));

  private:
  optional<Vec2> initialPos;
  PTask task;
  optional<FilePath> splashPath;
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
  SERIALIZE_ALL(SUBCLASS(Behaviour));
};

REGISTER_TYPE(Heal);
REGISTER_TYPE(Rest);
REGISTER_TYPE(MoveRandomly);
REGISTER_TYPE(BirdFlyAway);
REGISTER_TYPE(GoldLust);
REGISTER_TYPE(Wildlife);
REGISTER_TYPE(Fighter);
REGISTER_TYPE(FighterStandGround);
REGISTER_TYPE(GuardTarget);
REGISTER_TYPE(GuardArea);
REGISTER_TYPE(GuardSquare);
REGISTER_TYPE(Summoned);
REGISTER_TYPE(DieTime);
REGISTER_TYPE(Wait);
REGISTER_TYPE(Thief);
REGISTER_TYPE(ByCollective);
REGISTER_TYPE(ChooseRandom);
REGISTER_TYPE(SingleTask);
REGISTER_TYPE(AvoidFire);
REGISTER_TYPE(StayOnFurniture);

MonsterAI::MonsterAI(WCreature c, const vector<Behaviour*>& beh, const vector<int>& w, bool pick) :
    weights(w), creature(c), pickItems(pick) {
  CHECK(beh.size() == w.size());
  for (auto b : beh)
    behaviours.push_back(PBehaviour(b));
}

void MonsterAI::makeMove() {
  vector<MoveInfo> moves;
  for (int i : All(behaviours)) {
    MoveInfo move = behaviours[i]->getMove();
    move.setValue(move.getValue() * weights[i]);
    moves.push_back(move);
    bool skipNextMoves = false;
    if (i < behaviours.size() - 1) {
      CHECK(weights[i] >= weights[i + 1]);
      if (moves.back().getValue() > weights[i + 1])
        skipNextMoves = true;
    }
    if (pickItems)
      for (auto& stack : Item::stackItems(creature->getPickUpOptions())) {
        WItem item = stack[0];
        if (!item->isOrWasForSale() && creature->pickUp(stack))
          moves.push_back(MoveInfo({ behaviours[i]->itemValue(item) * weights[i], creature->pickUp(stack)}));
      }
    if (skipNextMoves)
      break;
  }
  /*vector<WItem> inventory = creature->getEquipment().getItems([this](WItem item) { return !creature->getEquipment().isEquiped(item);});
  for (WItem item : inventory) {
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
  for (int i : All(moves))
    if (moves[i].getValue() > winner.getValue())
      winner = moves[i];
  CHECK(winner.getValue() > 0);
  winner.getMove().perform(creature);
}

PMonsterAI MonsterAIFactory::getMonsterAI(WCreature c) const {
  return PMonsterAI(maker(c));
}

MonsterAIFactory::MonsterAIFactory(MakerFun _maker) : maker(_maker) {
}

MonsterAIFactory MonsterAIFactory::guard() {
  return MonsterAIFactory([=](WCreature c) {
      vector<Behaviour*> actors {
          new AvoidFire(c),
          new Heal(c),
          new FighterStandGround(c, 0.6),
          new Wait(c)
      };
      vector<int> weights { 10, 5, 4, 1 };
      return new MonsterAI(c, actors, weights);
  });
}

MonsterAIFactory MonsterAIFactory::monster() {
  return stayInLocation(Level::getMaxBounds());
}

MonsterAIFactory MonsterAIFactory::collective(WCollective col) {
  return MonsterAIFactory([=](WCreature c) {
      return new MonsterAI(c, {
        new AvoidFire(c),
        new Heal(c),
        new ByCollective(c, col, unique<Fighter>(c, 0.6)),
        new ChooseRandom(c, makeVec(PBehaviour(new Rest(c)), PBehaviour(new MoveRandomly(c))), {3, 1})},
        { 10, 6, 2, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::stayInLocation(Rectangle rect, bool moveRandomly) {
  return MonsterAIFactory([=](WCreature c) {
      vector<Behaviour*> actors { 
          new AvoidFire(c),
          new Heal(c),
          new Thief(c),
          new Fighter(c, 0.6),
          new GoldLust(c),
          new GuardArea(c, rect)
      };
      vector<int> weights { 10, 5, 4, 3, 1, 1 };
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

MonsterAIFactory MonsterAIFactory::singleTask(PTask&& t, bool chaseEnemies) {
  // Since the lambda can't capture OwnedPointer, let's release it to shared_ptr and recreate PTask inside the lambda.
  auto released = t.giveMeSharedPointer();
  return MonsterAIFactory([=](WCreature c) mutable {
      CHECK(released);
      auto task = PTask(released);
      released = nullptr;
      return new MonsterAI(c, {
        new Heal(c),
        chaseEnemies ? (Behaviour*)(new Fighter(c, 0.6)) : (Behaviour*)(new FighterStandGround(c, 0.6)),
        new SingleTask(c, std::move(task)),
        new ChooseRandom(c, makeVec(PBehaviour(new Rest(c)), PBehaviour(new MoveRandomly(c))), {3, 1})},
        { 6, 5, 2, 1}, true);
      });
}

MonsterAIFactory MonsterAIFactory::wildlifeNonPredator() {
  return MonsterAIFactory([](WCreature c) {
      return new MonsterAI(c, {
          new Wildlife(c),
          new FighterStandGround(c, 1.2),
          new MoveRandomly(c)},
          {6, 5, 1});
      });
}

MonsterAIFactory MonsterAIFactory::moveRandomly() {
  return MonsterAIFactory([](WCreature c) {
      return new MonsterAI(c, {
          new MoveRandomly(c)},
          {1});
      });
}

MonsterAIFactory MonsterAIFactory::stayOnFurniture(FurnitureType type) {
  return MonsterAIFactory([type](WCreature c) {
      return new MonsterAI(c, {
          new AvoidFire(c),
          new Fighter(c, 0.6),
          new StayOnFurniture(c, type)},
          {5, 2, 1});
      });
}

MonsterAIFactory MonsterAIFactory::idle() {
  return MonsterAIFactory([](WCreature c) {
      return new MonsterAI(c, {
          new Rest(c)},
          {1});
      });
}

MonsterAIFactory MonsterAIFactory::scavengerBird(Position corpsePos) {
  return MonsterAIFactory([=](WCreature c) {
      return new MonsterAI(c, {
          new GuardSquare(c, corpsePos, 1, 2),
          new BirdFlyAway(c, 3),
          new MoveRandomly(c),
      },
      {2, 1, 1});
      });
}

MonsterAIFactory MonsterAIFactory::summoned(WCreature leader) {
  return MonsterAIFactory([=](WCreature c) {
      return new MonsterAI(c, {
          new Summoned(c, leader, 1, 3),
          new AvoidFire(c),
          new Heal(c),
          new Fighter(c, 0.6),
          new MoveRandomly(c),
          new GoldLust(c)},
          { 6, 5, 4, 3, 1, 1 });
      });
}

MonsterAIFactory MonsterAIFactory::splashHeroes(bool leader) {
  return MonsterAIFactory([=](WCreature c) {
      return new MonsterAI(c, {
        leader ? (Behaviour*)new SplashHeroLeader(c) : (Behaviour*)new SplashHeroes(c),
        new Heal(c),
        new Fighter(c, 0.6),
        new ChooseRandom(c, makeVec(PBehaviour(new Rest(c)), PBehaviour(new MoveRandomly(c))), {3, 1})},
        { 6, 5, 2, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::splashMonsters() {
  return MonsterAIFactory([=](WCreature c) {
      return new MonsterAI(c, {
        new SplashMonsters(c),
        new Heal(c),
        new Fighter(c, 0.6),
        new ChooseRandom(c, makeVec(PBehaviour(new Rest(c)), PBehaviour(new MoveRandomly(c))), {3, 1})},
        { 6, 5, 2, 1}, false);
      });
}

MonsterAIFactory MonsterAIFactory::splashImps(const FilePath& splashPath) {
  return MonsterAIFactory([=](WCreature c) {
      return new MonsterAI(c, {
        new SplashImps(c, splashPath),
        new Heal(c),
        new Fighter(c, 0.6),
        new ChooseRandom(c, makeVec(PBehaviour(new Rest(c)), PBehaviour(new MoveRandomly(c))), {3, 1})},
        { 6, 5, 2, 1}, false);
      });
}

