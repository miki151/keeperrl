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

#include "enums.h"
#include "stdafx.h"

#include "task.h"
#include "level.h"
#include "entity_set.h"
#include "item.h"
#include "creature.h"
#include "collective.h"
#include "equipment.h"
#include "tribe.h"
#include "creature_name.h"
#include "collective_teams.h"
#include "effect.h"
#include "creature_group.h"
#include "player_message.h"
#include "territory.h"
#include "item_factory.h"
#include "game.h"
#include "model.h"
#include "collective_name.h"
#include "creature_attributes.h"
#include "body.h"
#include "furniture_type.h"
#include "furniture_factory.h"
#include "construction_map.h"
#include "furniture.h"
#include "monster_ai.h"
#include "vision.h"
#include "position_matching.h"
#include "navigation_flags.h"
#include "storage_info.h"
#include "content_factory.h"
#include "creature_list.h"
#include "automaton_part.h"
#include "spell_map.h"
#include "keybinding.h"

template <class Archive>
void Task::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(OwnedObject<Task>) & SUBCLASS(UniqueEntity);
  ar(done, transfer, viewId);
}

SERIALIZABLE(Task);

Task::Task(bool t) : transfer(t) {
}

Task::~Task() {
}

bool Task::isBogus() const {
  return false;
}

bool Task::canTransfer() {
  return transfer;
}

bool Task::canPerform(const Creature* c, const MovementType& type) const {
  return canPerformImpl(c, type) == TaskPerformResult::YES;
}

TaskPerformResult Task::canPerformImpl(const Creature* c, const MovementType&) const {
  return canPerformByAnyone() ? TaskPerformResult::YES : TaskPerformResult::NOT_NOW;
}

bool Task::canPerform(const Creature* c) const {
  return canPerform(c, c->getMovementType());
}

bool Task::canPerformByAnyone() const {
  return true;
}

optional<Position> Task::getPosition() const {
  return none;
}

optional<StorageId> Task::getStorageId(bool dropOnly) const {
  return none;
}

optional<ViewId> Task::getViewId() const {
  return viewId;
}

bool Task::isDone() const {
  return isBogus() || done;
}

void Task::setViewId(ViewId id) {
  viewId = id;
}

void Task::setDone() {
  done = true;
}

namespace {

class Construction : public Task {
  public:
  Construction(WTaskCallback c, Position pos, FurnitureType type) : Task(true), furnitureType(type), position(pos),
      callback(c) {}

  virtual bool isBogus() const override {
    return !position.canConstruct(furnitureType);
  }

  virtual string getDescription() const override {
    return "Build " + position.getGame()->getContentFactory()->furniture.getData(furnitureType).getName() + " at " + toString(position);
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!callback->isConstructionReachable(position))
      return NoMove;
    if (c->getPosition().dist8(position).value_or(2) > 1)
      return c->moveTowards(position);
    Vec2 dir = c->getPosition().getDir(position);
    if (auto action = c->construct(dir, furnitureType))
      return {1.0, action.append([=](Creature* c) {
          if (!position.isActiveConstruction(position.getGame()->getContentFactory()->furniture.getData(furnitureType).getLayer())) {
            setDone();
            callback->onConstructed(position, furnitureType);
          }
          })};
    else {
      setDone();
      return NoMove;
    }
  }

  SERIALIZE_ALL(SUBCLASS(Task), position, furnitureType, callback)
  SERIALIZATION_CONSTRUCTOR(Construction)

  private:
  FurnitureType SERIAL(furnitureType);
  Position SERIAL(position);
  WTaskCallback SERIAL(callback) = nullptr;
};

}

PTask Task::construction(WTaskCallback c, Position target, FurnitureType type) {
  return makeOwner<Construction>(c, target, type);
}

namespace {
class Destruction : public Task {
  public:
  Destruction(WTaskCallback c, Position pos, const Furniture* furniture, DestroyAction action, PositionMatching* m)
      : Task(true), position(pos), callback(c), destroyAction(action),
        description(action.getVerbSecondPerson() + " "_s + furniture->getName() + " at " + toString(position)),
        furnitureType(furniture->getType()), matching(m) {
    if (matching)
      matching->addTarget(position);
  }

  const Furniture* getFurniture() const {
    return position.getFurniture(position.getGame()->getContentFactory()->furniture.getData(furnitureType).getLayer());
  }

  virtual bool isBogus() const override {
    if (auto furniture = getFurniture())
      if (furniture->canDestroy(destroyAction))
        return false;
    return true;
  }

  virtual bool canPerformByAnyone() const override {
    PROFILE_BLOCK("Destruction::canPerform");
    return callback->isConstructionReachable(position) && (!matching || matching->getMatch(position));
  }

  virtual string getDescription() const override {
    return description;
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!canPerform(c))
      return NoMove;
    if (matching) {
      auto match = *matching->getMatch(position);
      if (c->getPosition() != match) {
        if (c->canNavigateTo(match))
          return c->moveTowards(*matching->getMatch(position));
      }
    }
    if (c->getPosition().dist8(position).value_or(2) > 1)
      return c->moveTowards(position);
    Vec2 dir = c->getPosition().getDir(position);
    CHECK(dir.length8() <= 1);
    if (auto action = c->destroy(dir, destroyAction))
      return {1.0, action.append([=](Creature* c) {
          if (!getFurniture() || getFurniture()->getType() != furnitureType) {
            setDone();
            callback->onDestructed(position, furnitureType, destroyAction);
          }
          })};
    else {
      setDone();
      return NoMove;
    }
  }

  ~Destruction() override {
    if (matching)
      matching->releaseTarget(position);
  }

  SERIALIZE_ALL(SUBCLASS(Task), position, callback, destroyAction, description, furnitureType, matching)
  SERIALIZATION_CONSTRUCTOR(Destruction)

  private:
  Position SERIAL(position);
  WTaskCallback SERIAL(callback) = nullptr;
  DestroyAction SERIAL(destroyAction);
  string SERIAL(description);
  FurnitureType SERIAL(furnitureType);
  PositionMatching* SERIAL(matching) = nullptr;
};

}

PTask Task::destruction(WTaskCallback c, Position target, const Furniture* furniture, DestroyAction destroyAction, PositionMatching* matching) {
  return makeOwner<Destruction>(c, target, furniture, destroyAction, matching);
}

namespace {

class EquipItem : public Task {
  public:
  EquipItem(Item* item) : item(item->getUniqueId()), itemName(item->getName()) {
  }

  virtual string getDescription() const override {
    return "Equip " + itemName;
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (auto it = c->getEquipment().getItemById(item))
      if (auto action = c->equip(it))
        return {1.0, action.append([=](Creature*) {
          setDone();
        })};
    setDone();
    return NoMove;
  }

  SERIALIZE_ALL(SUBCLASS(Task), item, itemName);
  SERIALIZATION_CONSTRUCTOR(EquipItem);

  private:
  UniqueEntity<Item>::Id SERIAL(item);
  string SERIAL(itemName);
};

}

PTask Task::pickAndEquipItem(Position position, Item* item) {
  return chain(pickUpItem(position, {item}), equipItem(item));
}

PTask Task::equipItem(Item* item) {
  return makeOwner<EquipItem>(item);
}

template <typename PositionContainer>
static optional<Position> chooseRandomClose(const Creature* c, const PositionContainer& squares, Task::SearchType type,
    bool stepOn) {
  auto canNavigate = [&] (const Position& pos) {
    auto other = pos.getCreature();
    return (!other || !LastingEffects::restrictedMovement(other))
        && (stepOn ? c->canNavigateTo(pos) : c->canNavigateToOrNeighbor(pos));
  };
  int minD = 10000000;
  int margin = type == Task::LAZY ? 0 : 3;
  vector<Position> close;
  auto start = c->getPosition();
  for (auto& v : squares)
    if (canNavigate(v))
      // positions on another level are the worst but still acceptable
      minD = min(minD, v.dist8(start).value_or(10000));
  for (auto& v : squares)
    if (canNavigate(v) && v.dist8(start).value_or(10000) <= minD + margin)
      close.push_back(v);
  if (!close.empty())
    return Random.choose(close);
  else
    return none;
}

namespace {
class GoToAnd : public Task {
  public:
  GoToAnd(vector<Position> targets, PTask task) : targets(targets), task(std::move(task)) {}

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType& type) const override {
    auto mainRes = task->canPerformImpl(c, type);
    if (mainRes == TaskPerformResult::NEVER)
      return TaskPerformResult::NEVER;
    for (auto pos : targets) {
      if (pos == c->getPosition())
        return mainRes;
      if (c->canNavigateTo(pos))
        return TaskPerformResult::YES;
    }
    return TaskPerformResult::NOT_NOW;
  }

  virtual string getDescription() const override {
    return task->getDescription();
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (task->isDone()) {
      setDone();
      return NoMove;
    }
    if (targets.contains(c->getPosition()))
      return task->getMove(c);
    if (!target)
      target = chooseRandomClose(c, targets, Task::SearchType::LAZY, true);
    if (!target) {
      setDone();
      return NoMove;
    } else
      return c->moveTowards(*target, NavigationFlags().requireStepOnTile());
  }

  SERIALIZATION_CONSTRUCTOR(GoToAnd)
  SERIALIZE_ALL(SUBCLASS(Task), target, targets, task)

  private:
  optional<Position> SERIAL(target);
  vector<Position> SERIAL(targets);
  PTask SERIAL(task);
};
}

class ApplySquare : public Task {
  public:
  using PositionInfo = pair<Position, FurnitureLayer>;
  ApplySquare(WTaskCallback c, vector<PositionInfo> pos, SearchType t, ActionType a)
      : positions(pos), callback(c), searchType(t), actionType(a) {}

  void changePosIfOccupied() {
    if (position)
      if (Creature* c = position->first.getCreature())
        if (LastingEffects::restrictedMovement(c))
          position = none;
  }

  optional<PositionInfo> choosePosition(Creature* c) {
    vector<Position> candidates;
    for (auto& pos : positions) {
      if (Creature* other = pos.first.getCreature())
        if (LastingEffects::restrictedMovement(other))
          continue;
      if (!rejectedPosition.count(pos.first))
        candidates.push_back(pos.first);
    }
    if (auto res = chooseRandomClose(c, candidates, searchType, false))
      for (auto& pos : positions)
        if (pos.first == *res)
          return pos;
    return none;
  }

  virtual MoveInfo getMove(Creature* c) override {
  PROFILE;
    changePosIfOccupied();
    if (!position) {
      if (auto pos = choosePosition(c))
        position = pos;
      else {
        setDone();
        return NoMove;
      }
    }
    if (atTarget(c)) {
      if (auto action = getAction(c))
        return {1.0, action.append([=](Creature* c) {
            setDone();
            callback->onAppliedSquare(c, *position);
        })};
      else {
        setDone();
        return NoMove;
      }
    } else {
      MoveInfo move(c->moveTowards(position->first));
      if (!move || (position->first.dist8(c->getPosition()) == 1 && position->first.getCreature() &&
          LastingEffects::restrictedMovement(position->first.getCreature()))) {
        rejectedPosition.insert(position->first);
        position = none;
        if (--invalidCount == 0) {
          setDone();
        }
        return NoMove;
      }
      else
        return move;
    }
  }

  CreatureAction getAction(Creature* c) {
    switch (actionType) {
      case ActionType::APPLY:
        return c->applySquare(position->first, position->second);
      case ActionType::NONE:
        return c->wait();
    }
  }

  virtual string getDescription() const override {
    return "Apply square " + (position ? toString(position->first) : "");
  }

  bool atTarget(Creature* c) {
    return position->first == c->getPosition() ||
        (!position->first.canEnterEmpty(c) && position->first.dist8(c->getPosition()) == 1);
  }

  SERIALIZE_ALL(SUBCLASS(Task), positions, rejectedPosition, invalidCount, position, callback, searchType, actionType)
  SERIALIZATION_CONSTRUCTOR(ApplySquare)

  private:
  vector<pair<Position, FurnitureLayer>> SERIAL(positions);
  PositionSet SERIAL(rejectedPosition);
  int SERIAL(invalidCount) = 5;
  optional<pair<Position, FurnitureLayer>> SERIAL(position);
  WTaskCallback SERIAL(callback) = nullptr;
  SearchType SERIAL(searchType);
  ActionType SERIAL(actionType);
};

PTask Task::applySquare(WTaskCallback c, vector<pair<Position, FurnitureLayer>> position, SearchType searchType,
    ActionType actionType) {
  CHECK(position.size() > 0);
  return makeOwner<ApplySquare>(c, position, searchType, actionType);
}

namespace {

class ArcheryRange : public Task {
  public:
  ArcheryRange(WTaskCallback c, vector<Position> pos) : callback(c), targets(pos) {}

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType&) const override {
    return !!getShootInfo(c) ? TaskPerformResult::YES : TaskPerformResult::NOT_NOW;
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (Random.roll(50))
      shootInfo = none;
    if (!shootInfo)
      shootInfo = getShootInfo(c);
    if (!shootInfo) {
      setDone();
      return NoMove;
    }
    if (c->getPosition() != shootInfo->pos)
      return c->moveTowards(shootInfo->pos, NavigationFlags().requireStepOnTile());
    if (Random.roll(3))
      return c->wait();
    for (auto pos = shootInfo->pos; pos != shootInfo->target; pos = pos.plus(shootInfo->dir)) {
      if (auto other = pos.plus(shootInfo->dir).getCreature())
        if (other->isFriend(c))
          return c->wait();
    }
    for (auto spell : c->getSpellMap().getAvailable(c))
      if (spell->getRange() > 1 && !spell->isEndOnly())  
        if (auto move = c->castSpell(spell, shootInfo->target))
          return move.append(
              [this, target = shootInfo->target](Creature* c) {
                callback->onAppliedSquare(c, make_pair(target, FurnitureLayer::MIDDLE));
                setDone();
              });
    return NoMove;
  }

  virtual string getDescription() const override {
    return "Shoot at ..";
  }

  SERIALIZE_ALL(SUBCLASS(Task), callback, targets, shootInfo)
  SERIALIZATION_CONSTRUCTOR(ArcheryRange)

  private:
  WTaskCallback SERIAL(callback) = nullptr;
  vector<Position> SERIAL(targets);
  struct ShootInfo {
    Position SERIAL(pos);
    Position SERIAL(target);
    Vec2 SERIAL(dir);
    SERIALIZE_ALL(pos, target, dir)
  };
  optional<ShootInfo> SERIAL(shootInfo);

  optional<ShootInfo> getShootInfo(const Creature* c) const {
    auto getDir = [&](Position target) -> optional<ShootInfo> {
      for (Vec2 dir : Vec2::directions4(Random)) {
        bool ok = true;
        for (int i : Range(Task::archeryRangeDistance))
          if (target.minus(dir * (i + 1)).stopsProjectiles(c->getVision().getId())) {
            ok = false;
            break;
          }
        auto pos = target.minus(dir * Task::archeryRangeDistance);
        if (ok && c->canNavigateTo(pos))
          return ShootInfo{pos, target, dir};
      }
      return none;
    };
    HashMap<Position, vector<ShootInfo>> shootPositions;
    for (auto pos : Random.permutation(targets))
      if (auto dir = getDir(pos))
        shootPositions[dir->pos].push_back(*dir);
    if (auto chosen = chooseRandomClose(c, getKeys(shootPositions), Task::RANDOM_CLOSE, true))
      return Random.choose(shootPositions.at(*chosen));
    return none;
  }
};

}

PTask Task::archeryRange(WTaskCallback c, vector<Position> positions) {
  return makeOwner<ArcheryRange>(c, positions);
}

namespace {

class Kill : public Task {
  public:
  enum Type { ATTACK, TORTURE, DISASSEMBLE };
  Kill(Creature* c, Type t) : Task(true), creature(c), type(t) {}

  CreatureAction getAction(Creature* c) {
    switch (type) {
      case ATTACK: return c->execute(creature.get(), "execute", "executes");
      case DISASSEMBLE: return c->execute(creature.get(), "disassemble", "disassembles");
      case TORTURE: return c->torture(creature.get());
    }
  }

  virtual string getDescription() const override {
    switch (type) {
      case ATTACK: return "Kill " + creature->getName().bare();
      case TORTURE: return "Torture " + creature->getName().bare();
      case DISASSEMBLE: return "Disassemble " + creature->getName().bare();
    }
  }

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType&) const override {
    return (creature == c || LastingEffects::restrictedMovement(c)) ? TaskPerformResult::NEVER : TaskPerformResult::YES;
  }

  virtual MoveInfo getMove(Creature* c) override {
    CHECK(creature != c);
    if (!creature || creature->isDead() || (type == TORTURE && !creature->isAffected(LastingEffect::TIED_UP))) {
      setDone();
      return NoMove;
    }
    if (auto action = getAction(c))
      return action.append([=](Creature* c) { if (creature->isDead()) setDone(); });
    else
      return c->moveTowards(creature->getPosition());
  }

  SERIALIZE_ALL(SUBCLASS(Task), creature, type)
  SERIALIZATION_CONSTRUCTOR(Kill)

  private:
  WeakPointer<Creature> SERIAL(creature);
  Type SERIAL(type);
};

}

PTask Task::kill(Creature* creature) {
  return makeOwner<Kill>(creature, Kill::ATTACK);
}

PTask Task::torture(Creature* creature) {
  return makeOwner<Kill>(creature, Kill::TORTURE);
}

PTask Task::disassemble(Creature* creature) {
  return makeOwner<Kill>(creature, Kill::DISASSEMBLE);
}

namespace {

class Disappear : public Task {
  public:
  Disappear() {}

  virtual MoveInfo getMove(Creature* c) override {
    return c->disappear();
  }

  virtual string getDescription() const override {
    return "Disappear";
  }

  SERIALIZE_ALL(SUBCLASS(Task))
};

}

PTask Task::disappear() {
  return makeOwner<Disappear>();
}

namespace {

class Chain : public Task {
  public:
  Chain(vector<PTask> t) : tasks(std::move(t)) {
    CHECK(!tasks.empty());
  }

  virtual bool isBogus() const override {
    return current >= tasks.size();
  }

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType& type) const override {
    if (current >= tasks.size())
      return TaskPerformResult::NEVER;
    for (int i : Range(current + 1, tasks.size()))
      if (tasks[i]->canPerformImpl(c, type) == TaskPerformResult::NEVER)
        return TaskPerformResult::NEVER;
    return tasks[current]->canPerformImpl(c, type);
  }

  virtual bool canTransfer() override {
    if (current >= tasks.size())
      return false;
    return tasks[current]->canTransfer();
  }

  virtual void cancel() override {
    for (int i = current; i < tasks.size(); ++i)
      if (!tasks[i]->isDone())
        tasks[i]->cancel();
  }

  virtual MoveInfo getMove(Creature* c) override {
    while (tasks[current]->isDone())
      if (++current >= tasks.size()) {
        setDone();
        return NoMove;
      }
    return tasks[current]->getMove(c);
  }

  virtual string getDescription() const override {
    if (current >= tasks.size())
      return "Chain: done";
    return tasks[current]->getDescription();
  }

  SERIALIZE_ALL(SUBCLASS(Task), tasks, current)
  SERIALIZATION_CONSTRUCTOR(Chain)

  private:
  vector<PTask> SERIAL(tasks);
  int SERIAL(current) = 0;
};

}

PTask Task::chain(PTask t1, PTask t2) {
  return makeOwner<Chain>(makeVec(std::move(t1), std::move(t2)));
}

PTask Task::chain(PTask t1, PTask t2, PTask t3) {
  return makeOwner<Chain>(makeVec(std::move(t1), std::move(t2), std::move(t3)));
}

PTask Task::chain(vector<PTask> v) {
  return makeOwner<Chain>(std::move(v));
}

namespace {

class Explore : public Task {
  public:
  Explore(Position pos) : position(pos) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!Random.roll(3))
      return NoMove;
    if (auto action = c->moveTowards(position))
      return action.append([=](Creature* c) {
          if (c->getPosition().dist8(position).value_or(5) < 5)
            setDone();
      });
    if (Random.roll(3))
      setDone();
    return NoMove;
  }

  virtual string getDescription() const override {
    return "Explore " + toString(position);
  }

  SERIALIZE_ALL(SUBCLASS(Task), position)
  SERIALIZATION_CONSTRUCTOR(Explore)

  private:
  Position SERIAL(position);
};

}

PTask Task::explore(Position pos) {
  return makeOwner<Explore>(pos);
}

namespace {

class AbuseMinion : public Task {
  public:
  AbuseMinion(Collective* col, Creature* target) : collective(col), target(target) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!target || !collective->getTerritory().contains(target->getPosition())) {
      setDone();
      return NoMove;
    }
    if (c->getPosition().dist8(target->getPosition()).value_or(2) > 1) {
      if (auto action = c->moveTowards(target->getPosition()))
        return action;
      setDone();
      return NoMove;
    } else {
      return c->whip(target->getPosition(), 1.0).append([this](Creature*) {
        target->addEffect(LastingEffect::SPEED, 10_visible); setDone();
      });
    }
  }

  virtual string getDescription() const override {
    return "Abuse " + target->getName().bare();
  }

  SERIALIZE_ALL(SUBCLASS(Task), collective, target)
  SERIALIZATION_CONSTRUCTOR(AbuseMinion)

  private:
  Collective* SERIAL(collective);
  WeakPointer<Creature> SERIAL(target);
};

}

PTask Task::abuseMinion(Collective* col, Creature* c) {
  return makeOwner<AbuseMinion>(col, c);
}

namespace {

class AttackCreatures : public Task {
  public:
  AttackCreatures(vector<Creature*> v) : creatures(v.transform([](auto c) { return c->getThis(); })) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (auto target = getNextCreature(c))
      return c->moveTowards(target->getPosition());
    return NoMove;
  }

  Creature* getNextCreature(Creature* attacker) const {
    for (auto c : creatures)
      // Temporarily disabled. Make the attackers search territory if no visible target exists
      if (c && !c->isDead() /*&& (attacker->canSeeInPosition(c.get(), attacker->getGame()->getGlobalTime()) ||
          attacker->isAffected(LastingEffect::TELEPATHY))*/)
        return c.get();
    return nullptr;
  }

  virtual string getDescription() const override {
    return "Attack someone";
  }

  SERIALIZE_ALL(SUBCLASS(Task), creatures)
  SERIALIZATION_CONSTRUCTOR(AttackCreatures)

  private:
  vector<WeakPointer<Creature>> SERIAL(creatures);
};

}

PTask Task::attackCreatures(vector<Creature*> c) {
  return makeOwner<AttackCreatures>(std::move(c));
}

PTask Task::stealFrom(Collective* collective, CollectiveResourceId id) {
  vector<PTask> tasks;
  auto& info = collective->getResourceInfo(id);
  for (auto storageId : info.storage)
    for (auto& pos : collective->getStoragePositions(storageId)) {
      vector<Item*> gold = pos.getItems(id);
      if (!gold.empty())
        tasks.push_back(pickUpItem(pos, gold));
    }
  if (!tasks.empty())
    return chain(std::move(tasks));
  else
    return PTask(nullptr);
}

namespace {

class CampAndSpawnTask : public Task {
  public:
  CampAndSpawnTask(Collective* _target, CreatureList s, int numAtt)
    : target(_target), spawns(s),
      campPos(Random.permutation(target->getTerritory().getStandardExtended())), numAttacks(numAtt) {}

  void updateTeams() {
    for (auto member : copyOf(attackTeam))
      if (member->isDead())
        attackTeam.removeElement(member);
  }

  virtual void cancel() override {
    // Cancel only the attack team, as the defense will disappear when the summoner dies
    for (Creature* c : attackTeam)
      if (!c->isDead())
        c->dieNoReason(Creature::DropType::NOTHING);
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (target->getLeaders().empty() || campPos.empty()) {
      setDone();
      return NoMove;
    }
    updateTeams();
    if (!campPos.contains(c->getPosition()))
      return c->moveTowards(campPos[0]);
    if (attackTeam.empty()) {
      if (!attackCountdown) {
        if (numAttacks-- <= 0) {
          setDone();
          return c->wait();
        }
        attackCountdown = Random.get(30, 60);
      }
      if (*attackCountdown > 0)
        --*attackCountdown;
      else {
        auto team = spawns.generate(Random, &c->getGame()->getContentFactory()->getCreatures(), c->getTribeId(),
            MonsterAIFactory::singleTask(Task::attackCreatures(target->getLeaders())));
        for (Creature* summon : Effect::summonCreatures(c->getPosition(), std::move(team)))
          attackTeam.push_back(summon);
        attackCountdown = none;
      }
    }
    return c->wait();
  }

  virtual string getDescription() const override {
    return "Camp and spawn";
  }

  SERIALIZE_ALL(SUBCLASS(Task), target, spawns, campPos, attackCountdown, attackTeam, numAttacks)
  SERIALIZATION_CONSTRUCTOR(CampAndSpawnTask)

  private:
  Collective* SERIAL(target) = nullptr;
  CreatureList SERIAL(spawns);
  vector<Position> SERIAL(campPos);
  optional<int> SERIAL(attackCountdown);
  vector<Creature*> SERIAL(attackTeam);
  int SERIAL(numAttacks);
};


}

PTask Task::campAndSpawn(Collective* target, const CreatureList& spawns, int numAttacks) {
  return makeOwner<CampAndSpawnTask>(target, spawns, numAttacks);
}

namespace {

class KillFighters : public Task {
  public:
  KillFighters(Collective* col, int numC) : collective(col), numCreatures(numC) {}

  virtual MoveInfo getMove(Creature* c) override {
    optional<Position> moveTarget;
    auto process = [&] (const vector<Creature*>& creatures) {
      for (const Creature* target : creatures)
        // Temporarily disabled. Make the attackers search territory if no visible target exists
        /*if (c->canSeeInPosition(target, c->getGame()->getGlobalTime()) || c->isAffected(LastingEffect::TELEPATHY))*/
        {
          targets.insert(target);
          moveTarget = target->getPosition();
        }
    };
    process(collective->getCreatures(MinionTrait::FIGHTER));
    process(collective->getCreatures(MinionTrait::LEADER));
    int numKilled = 0;
    for (auto& info : c->getKills())
      if (targets.contains(info.creature))
        ++numKilled;
    if (numKilled >= numCreatures || !moveTarget) {
      setDone();
      return NoMove;
    }
    return c->moveTowards(*moveTarget);
  }

  virtual string getDescription() const override {
    return "Kill " + toString(numCreatures) + " minions of " + collective->getName()->full;
  }

  SERIALIZE_ALL(SUBCLASS(Task), collective, numCreatures, targets)
  SERIALIZATION_CONSTRUCTOR(KillFighters)

  private:
  Collective* SERIAL(collective) = nullptr;
  int SERIAL(numCreatures);
  EntitySet<Creature> SERIAL(targets);
};

}

PTask Task::killFighters(Collective* col, int numCreatures) {
  return makeOwner<KillFighters>(col, numCreatures);
}

namespace {
class ConsumeItem : public Task {
  public:
  ConsumeItem(WTaskCallback c, vector<Item*> _items) : items(_items), callback(c) {}

  virtual MoveInfo getMove(Creature* c) override {
    return c->wait().append([=](Creature* c) {
        c->getEquipment().removeItems(c->getEquipment().getItems().filter(items.containsPredicate()), c); });
  }

  virtual string getDescription() const override {
    return "Consume item";
  }

  SERIALIZE_ALL(SUBCLASS(Task), items, callback)
  SERIALIZATION_CONSTRUCTOR(ConsumeItem)

  protected:
  EntitySet<Item> SERIAL(items);
  WTaskCallback SERIAL(callback) = nullptr;
};
}

PTask Task::consumeItem(WTaskCallback c, vector<Item*> items) {
  return makeOwner<ConsumeItem>(c, items);
}

namespace {
class Copulate : public Task {
  public:
  Copulate(WTaskCallback c, Creature* t, int turns) : target(t), callback(c), numTurns(turns) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!target || target->isDead() || !target->isAffected(LastingEffect::SLEEP)) {
      setDone();
      return NoMove;
    }
    if (c->getPosition().dist8(target->getPosition()) == 1) {
      if (Random.roll(2))
        for (Vec2 v : Vec2::directions8(Random))
          if (v.dist8(c->getPosition().getDir(target->getPosition())) == 1)
            if (auto action = c->move(v))
              return action;
      if (auto action = c->copulate(c->getPosition().getDir(target->getPosition())))
        return action.append([=](Creature* c) {
          if (--numTurns == 0) {
            setDone();
            callback->onCopulated(c, target.get());
          }});
      else
        return NoMove;
    } else
      return c->moveTowards(target->getPosition());
  }

  virtual string getDescription() const override {
    return "Copulate with " + target->getName().bare();
  }

  SERIALIZE_ALL(SUBCLASS(Task), target, numTurns, callback)
  SERIALIZATION_CONSTRUCTOR(Copulate)

  protected:
  WeakPointer<Creature> SERIAL(target);
  WTaskCallback SERIAL(callback) = nullptr;
  int SERIAL(numTurns);
};
}

PTask Task::copulate(WTaskCallback c, Creature* target, int numTurns) {
  return makeOwner<Copulate>(c, target, numTurns);
}

namespace {
class Consume : public Task {
  public:
  Consume(Creature* t) : target(t) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!target || target->isDead()) {
      setDone();
      return NoMove;
    }
    if (c->getPosition().dist8(target->getPosition()) == 1) {
      if (auto action = c->consume(target.get()))
        return action.append([=](Creature*) {
          setDone();
        });
      else
        return NoMove;
    } else
      return c->moveTowards(target->getPosition());
  }

  virtual string getDescription() const override {
    return "Absorb " + target->getName().bare();
  }

  SERIALIZE_ALL(SUBCLASS(Task), target)
  SERIALIZATION_CONSTRUCTOR(Consume)

  protected:
  WeakPointer<Creature> SERIAL(target);
};
}

PTask Task::consume(Creature* target) {
  return makeOwner<Consume>(target);
}

namespace {

class Eat : public Task {
  public:
  Eat(vector<Position> pos) : positions(pos) {}

  Item* getDeadChicken(Position pos) {
    vector<Item*> chickens = pos.getItems().filter(Item::classPredicate(ItemClass::FOOD));
    if (chickens.empty())
      return nullptr;
    else
      return chickens[0];
  }

  virtual MoveInfo getMove(Creature* c) override {
    PROFILE;
    if (!!position && !c->canNavigateTo(*position))
      position = none;
    if (!position) {
      for (Position v : Random.permutation(positions))
        if (!rejectedPosition.count(v) && c->canNavigateTo(v) && (!position ||
              position->dist8(c->getPosition()).value_or(10000) > v.dist8(c->getPosition()).value_or(10000)))
          position = v;
      if (!position) {
        setDone();
        return NoMove;
      }
    }
    if (c->getPosition() != *position && getDeadChicken(*position))
      return c->moveTowards(*position);
    Item* chicken = getDeadChicken(c->getPosition());
    if (chicken)
      return c->eat(chicken).append([=] (Creature* c) {
        setDone();
      });
    for (Position pos : c->getPosition().neighbors8(Random)) {
      Item* chicken = getDeadChicken(pos);
      if (chicken)
        if (auto move = c->move(pos))
          return move;
      if (Creature* ch = pos.getCreature())
        if (ch->getBody().isFarmAnimal())
          if (auto move = c->attack(ch)) {
            return move.append([this, ch, pos] (Creature*) { if (ch->isDead()) position = pos; });
      }
    }
    if (positions.contains(c->getPosition()))
      for (auto chicken : c->getVisibleCreatures())
        if (chicken->getBody().isFarmAnimal())
          if (auto move = c->moveTowards(chicken->getPosition()))
            return move;
    if (c->getPosition() != *position)
      return c->moveTowards(*position);
    else
      position = none;
    return NoMove;
  }

  virtual string getDescription() const override {
    if (position)
      return "Eat at " + toString(*position);
    else
      return "Eat";
  }

  SERIALIZE_ALL(SUBCLASS(Task), position, positions, rejectedPosition)
  SERIALIZATION_CONSTRUCTOR(Eat)

  private:
  optional<Position> SERIAL(position);
  vector<Position> SERIAL(positions);
  PositionSet SERIAL(rejectedPosition);
};

}

PTask Task::eat(vector<Position> hatcherySquares) {
  return makeOwner<Eat>(hatcherySquares);
}

namespace {
class GoTo : public Task {
  public:
  GoTo(Position pos, bool forever) : target(pos), tryForever(forever) {}

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType&) const override {
    return LastingEffects::restrictedMovement(c) ? TaskPerformResult::NEVER : TaskPerformResult::YES;
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (c->getPosition() == target ||
        (c->getPosition().dist8(target) == 1 && !target.canEnter(c)) ||
        (!tryForever && !c->canNavigateToOrNeighbor(target))) {
      setDone();
      return NoMove;
    } else
      return c->moveTowards(target);
  }

  virtual optional<Position> getPosition() const override {
    return target;
  }

  virtual string getDescription() const override {
    return "Go to " + toString(target);
  }

  SERIALIZE_ALL(SUBCLASS(Task), target, tryForever)
  SERIALIZATION_CONSTRUCTOR(GoTo)

  protected:
  Position SERIAL(target);
  bool SERIAL(tryForever);
};
}

PTask Task::goTo(Position pos) {
  return makeOwner<GoTo>(pos, false);
}

PTask Task::goToTryForever(Position pos) {
  return makeOwner<GoTo>(pos, true);
}

namespace {
class Dance : public Task {
  public:
  Dance(Collective* col) : collective(col) {}

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType&) const override {
    return LastingEffects::restrictedMovement(c) ? TaskPerformResult::NEVER : TaskPerformResult::YES;
  }

  virtual MoveInfo getMove(Creature* c) override {
    setDone();
    if (auto target = collective->getDancing().getTarget(c)) {
      return c->moveTowards(*target).append([=] (Creature* c) { if (Random.roll(10)) c->addFX({FXName::MUSIC});});
    }
    return NoMove;
  }

  virtual string getDescription() const override {
    return "Dance";
  }

  SERIALIZE_ALL(SUBCLASS(Task), collective)
  SERIALIZATION_CONSTRUCTOR(Dance)

  protected:
  Collective* SERIAL(collective);
};
}

PTask Task::dance(Collective* col) {
  return makeOwner<Dance>(col);
}

namespace {
class StayIn : public Task {
  public:
  StayIn(vector<Position> pos) : target(std::move(pos)) {}

  virtual MoveInfo getMove(Creature* c) override {
    PROFILE_BLOCK("StayIn::getMove");
    auto pos = c->getPosition();
    if (Random.roll(15) && target.contains(pos)) {
      setDone();
      if (Random.roll(15))
        if (auto move = c->move(pos.plus(Vec2(Random.choose<Dir>()))))
          return move;
      if (Random.roll(100))
        if (auto move = c->moveTowards(Random.choose(target)))
          return move;
      return c->wait();
    }
    if (!currentTarget)
      for (int i : Range(100)) {
        currentTarget = Random.choose(target);
        if (currentTarget->canEnter(c))
          break;
        else
          currentTarget = none;
      }
    if (currentTarget && currentTarget->isSameModel(c->getPosition()))
      if (auto move = c->moveTowards(*currentTarget))
        return move;
    setDone();
    return NoMove;
  }

  virtual string getDescription() const override {
    return "Go to " + toString(currentTarget);
  }

  SERIALIZE_ALL(SUBCLASS(Task), target, currentTarget)
  SERIALIZATION_CONSTRUCTOR(StayIn);

  protected:
  vector<Position> SERIAL(target);
  optional<Position> SERIAL(currentTarget);
};
}

PTask Task::stayIn(vector<Position> pos) {
  CHECK(!pos.empty());
  return makeOwner<StayIn>(pos);
}

namespace {
class Idle : public Task {
  public:

  virtual MoveInfo getMove(Creature* c) override {
    setDone();
    return c->wait();
  }

  virtual string getDescription() const override {
    return "Idle";
  }

  SERIALIZE_ALL(SUBCLASS(Task))
  SERIALIZATION_CONSTRUCTOR(Idle);
};
}

PTask Task::idle() {
  return makeOwner<Idle>();
}

namespace {
class AlwaysDone : public Task {
  public:
  AlwaysDone(PTask t, PTaskPredicate predicate) : task(std::move(t)),
      donePredicate(std::move(predicate)) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (donePredicate->apply())
      setDone();
    return task->getMove(c);
  }

  virtual string getDescription() const override {
    return task->getDescription();
  }

  virtual bool isBogus() const override {
    return task->isBogus();
  }

  virtual bool canTransfer() override {
    return task->canTransfer();
  }

  virtual void cancel() override {
    task->cancel();
  }

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType& type) const override {
    return task->canPerformImpl(c, type);
  }

  virtual optional<Position> getPosition() const override {
    return task->getPosition();
  }

  SERIALIZE_ALL(SUBCLASS(Task), task, donePredicate)
  SERIALIZATION_CONSTRUCTOR(AlwaysDone)

  private:
  PTask SERIAL(task);
  PTaskPredicate SERIAL(donePredicate);
};
}

PTask Task::alwaysDone(PTask t) {
  return doneWhen(std::move(t), TaskPredicate::always());
}

PTask Task::doneWhen(PTask task, PTaskPredicate predicate) {
  return makeOwner<AlwaysDone>(std::move(task), std::move(predicate));
}

namespace {
class Follow : public Task {
  public:
  Follow(Creature* t) : target(t) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (target && target != c && !target->isDead()) {
      Position targetPos = target->getPosition();
      if (targetPos.dist8(c->getPosition()).value_or(3) < 3) {
        if (Random.roll(15))
          if (auto move = c->move(c->getPosition().plus(Vec2(Random.choose<Dir>()))))
            return move;
        return NoMove;
      }
      return c->moveTowards(targetPos);
    } else {
      setDone();
      return NoMove;
    }
  }

  virtual string getDescription() const override {
    return "Follow " + target->getName().bare();
  }

  WeakPointer<Creature> SERIAL(target);

  SERIALIZE_ALL(SUBCLASS(Task), target)
  SERIALIZATION_CONSTRUCTOR(Follow)
};
}

PTask Task::follow(Creature* c) {
  return makeOwner<Follow>(c);
}

namespace {
class TransferTo : public Task {
  public:
  TransferTo(Model* m) : model(m) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (c->getPosition().getModel() == model) {
      setDone();
      return NoMove;
    }
    if (!target)
      target = c->getGame()->getTransferPos(model, c->getPosition().getModel());
    if (c->getPosition() == target && c->getGame()->canTransferCreature(c, model)) {
      return c->wait().append([=] (Creature* c) { setDone(); c->getGame()->transferCreature(c, model); });
    } else
      return c->moveTowards(*target);
  }

  virtual string getDescription() const override {
    return "Go to site";
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int) {
    target = none;
    ar(SUBCLASS(Task), target, model);
  }

  SERIALIZATION_CONSTRUCTOR(TransferTo)

  protected:
  optional<Position> SERIAL(target);
  Model* SERIAL(model) = nullptr;
};
}

PTask Task::transferTo(Model* m) {
  return makeOwner<TransferTo>(m);
}

namespace {
class GoToAndWait : public Task {
  public:
  GoToAndWait(Position pos, TimeInterval waitT) : position(pos), waitTime(waitT) {}

  bool arrived(Creature* c) {
    return c->getPosition() == position ||
        (c->getPosition().dist8(position) == 1 && !position.canEnterEmpty(c));
  }

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType&) const override {
    return LastingEffects::restrictedMovement(c) ? TaskPerformResult::NEVER : TaskPerformResult::YES;
  }

  virtual optional<Position> getPosition() const override {
    return position;
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (maxTime && *c->getLocalTime() >= *maxTime) {
      setDone();
      return NoMove;
    }
    if (!arrived(c)) {
      auto ret = c->moveTowards(position);
      if (!ret) {
        if (!timeout)
          timeout = *c->getLocalTime() + 30_visible;
        else
          if (*c->getLocalTime() > *timeout) {
            setDone();
            return NoMove;
          }
      } else
        timeout = none;
      return ret;
    } else {
      if (!maxTime)
        maxTime = *c->getLocalTime() + waitTime;
      return c->wait();
    }
  }

  virtual string getDescription() const override {
    return "Go to and wait " + toString(position);
  }

  SERIALIZE_ALL(SUBCLASS(Task), position, waitTime, maxTime, timeout)
  SERIALIZATION_CONSTRUCTOR(GoToAndWait)

  private:
  Position SERIAL(position);
  TimeInterval SERIAL(waitTime);
  optional<LocalTime> SERIAL(maxTime);
  optional<LocalTime> SERIAL(timeout);
};
}

PTask Task::goToAndWait(Position pos, TimeInterval waitTime) {
  return makeOwner<GoToAndWait>(pos, waitTime);
}

namespace {
class Whipping : public Task {
  public:
  Whipping(Position pos, Creature* w) : position(pos), whipped(w) {}

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType&) const override {
    return whipped == c ? TaskPerformResult::NEVER : TaskPerformResult::YES;
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!whipped || whipped != position.getCreature() || !whipped->isAffected(LastingEffect::TIED_UP)) {
      setDone();
      return NoMove;
    }
    if (c->getPosition().dist8(position).value_or(2) > 1)
      return c->moveTowards(position);
    else
      return c->whip(whipped->getPosition(), 0.3);
  }

  virtual string getDescription() const override {
    return "Whipping " + whipped->getName().a();
  }

  SERIALIZE_ALL(SUBCLASS(Task), position, whipped)
  SERIALIZATION_CONSTRUCTOR(Whipping)

  protected:
  Position SERIAL(position);
  WeakPointer<Creature> SERIAL(whipped);
};
}

PTask Task::whipping(Position pos, Creature* whipped) {
  return makeOwner<Whipping>(pos, whipped);
}

namespace {
class DropItemsAnywhere : public Task {
  public:
  DropItemsAnywhere(EntitySet<Item> it) : items(it) {}

  virtual MoveInfo getMove(Creature* c) override {
    return c->drop(c->getEquipment().getItems().filter(items.containsPredicate())).append([=] (Creature*) { setDone(); });
  }

  virtual string getDescription() const override {
    return "Drop items anywhere";
  }

  SERIALIZE_ALL(SUBCLASS(Task), items)
  SERIALIZATION_CONSTRUCTOR(DropItemsAnywhere)

  protected:
  EntitySet<Item> SERIAL(items);
};
}

PTask Task::dropItemsAnywhere(vector<Item*> items) {
  return makeOwner<DropItemsAnywhere>(items);
}

namespace {
class DropItems : public Task {
  public:
  DropItems(EntitySet<Item> items, StorageId storage, Collective* collective, optional<Position> origin)
      : items(std::move(items)), positions(StorageInfo{storage, collective}), origin(origin) {}

  DropItems(EntitySet<Item> items, vector<Position> positions)
      : items(std::move(items)), positions(std::move(positions)) {}

  virtual string getDescription() const override {
    return "Drop items";
  }

  optional<Position> chooseTarget(Creature* c) const {
    return positions.visit(
        [&](const vector<Position>& v) { return chooseRandomClose(c, v, LAZY, true); },
        [&](const StorageInfo& info) {
          return chooseRandomClose(c, info.collective->getStoragePositions(info.storage), LAZY, true);
        }
    );
  }

  void onPickedUp(Creature* c) {
    pickedUpCreature = c;
  }

  virtual optional<StorageId> getStorageId(bool) const override {
    if (auto info = positions.getReferenceMaybe<StorageInfo>())
      return info->storage;
    else
      return none;
  }

  bool isBlocked(Position pos) const {
    auto c = pos.getCreature();
    return c && LastingEffects::restrictedMovement(c);
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!target || !c->canNavigateTo(*target) || isBlocked(*target))
      target = chooseTarget(c);
    if (!target)
      return c->drop(c->getEquipment().getItems().filter(items.containsPredicate())).append(
          [this] (Creature*) {
            setDone();
          });
    if (c->getPosition() == target) {
      vector<Item*> myItems = c->getEquipment().getItems().filter(items.containsPredicate());
      if (auto action = c->drop(myItems))
        return {1.0, action.append([=](Creature*) {setDone();})};
      else {
        setDone();
        return NoMove;
      }
    } else {
      if (c->getPosition().dist8(*target) == 1)
        if (Creature* other = target->getCreature())
          if (other->isAffected(LastingEffect::SLEEP))
            other->removeEffect(LastingEffect::SLEEP);
      return c->moveTowards(*target);
    }
  }

  virtual bool isBogus() const override {
    return origin && (pickedUpCreature || !origin->getInventory().containsAnyOf(items)) &&
        (!pickedUpCreature || !pickedUpCreature->getEquipment().containsAnyOf(items));
  }

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType&) const override {
    PROFILE_BLOCK("DropItems::canPerform");
    return (!origin || c == pickedUpCreature ) && c->getEquipment().containsAnyOf(items)
        ? TaskPerformResult::YES : TaskPerformResult::NOT_NOW;
  }

  SERIALIZE_ALL(SUBCLASS(Task), items, positions, target, origin, pickedUpCreature)
  SERIALIZATION_CONSTRUCTOR(DropItems)

  protected:
  EntitySet<Item> SERIAL(items);
  variant<StorageInfo, vector<Position>> SERIAL(positions);
  optional<Position> SERIAL(target);
  optional<Position> SERIAL(origin);
  WeakPointer<Creature> SERIAL(pickedUpCreature);
};
}

PTask Task::dropItems(vector<Item*> items, StorageId storage, Collective* collective) {
  return makeOwner<DropItems>(items, storage, collective, none);
}

PTask Task::dropItems(vector<Item*> items, vector<Position> positions) {
  return makeOwner<DropItems>(items, std::move(positions));
}

namespace {

class PickUpItem : public Task {
  public:
  PickUpItem(Position position, vector<Item*> items, optional<StorageId> storage, WeakPointer<DropItems> dropTask)
      : Task(true), items(std::move(items)), position(position), tries(10), storage(storage),
        dropTask(std::move(dropTask)) {
    CHECK(!items.empty());
    lightestItem = 10000000;
    for (auto& item : items)
      lightestItem = min(lightestItem, item->getWeight());
  }

  virtual void onPickedUp() {
    setDone();
  }

  virtual optional<StorageId> getStorageId(bool dropOnly) const override {
    if (dropOnly)
      return none;
    else
      return storage;
  }

  virtual string getDescription() const override {
    return "Pick up item " + toString(position);
  }

  virtual bool isBogus() const override {
    return !position.getInventory().containsAnyOf(items);
  }

  virtual MoveInfo getMove(Creature* c) override {
    CHECK(!pickedUp);
    if (c->getPosition() == position) {
      vector<Item*> hereItems;
      for (Item* it : c->getPickUpOptions())
        if (items.contains(it)) {
          hereItems.push_back(it);
          items.erase(it);
        }
      if (hereItems.empty()) {
        setDone();
        return NoMove;
      }
      items = hereItems;
      if (auto action = c->pickUp(hereItems))
        return {1.0, action.append([=](Creature* c) {
          pickedUp = true;
          onPickedUp();
          if (dropTask)
            dropTask->onPickedUp(c);
        })};
      else {
        setDone();
        return NoMove;
      }
    }
    if (auto action = c->moveTowards(position, NavigationFlags().requireStepOnTile()))
      return action;
    else if (--tries == 0)
      setDone();
    return NoMove;
  }

  virtual TaskPerformResult canPerformImpl(const Creature* c, const MovementType& movement) const override {
    PROFILE_BLOCK("PickUpItem::canPerform");
    if (LastingEffects::restrictedMovement(c))
      return TaskPerformResult::NEVER;
    return c->canCarryMoreWeight(lightestItem) && position.canNavigateTo(c->getPosition(), movement)
        ? TaskPerformResult::YES : TaskPerformResult::NOT_NOW;
  }

  SERIALIZE_ALL(SUBCLASS(Task), items, pickedUp, position, tries, lightestItem, storage, dropTask)
  SERIALIZATION_CONSTRUCTOR(PickUpItem)

  protected:
  EntitySet<Item> SERIAL(items);
  bool SERIAL(pickedUp) = false;
  Position SERIAL(position);
  int SERIAL(tries);
  double SERIAL(lightestItem);
  optional<StorageId> SERIAL(storage);
  WeakPointer<DropItems> SERIAL(dropTask);
};
}

PTask Task::pickUpItem(Position position, vector<Item*> items, optional<StorageId> storage) {
  return makeOwner<PickUpItem>(position, items, storage, nullptr);
}

Task::PickUpAndDrop Task::pickUpAndDrop(Position origin, vector<Item*> items, StorageId storage, Collective* col) {
  auto drop = makeOwner<DropItems>(items, storage, col, origin);
  auto pickUp = makeOwner<PickUpItem>(origin, items, storage, drop.get());
  return PickUpAndDrop { std::move(pickUp), std::move(drop)};
}

namespace {
class Spider : public Task {
  public:
  Spider(Position orig, const vector<Position>& pos)
      : origin(orig), webPositions(pos) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!c->getPosition().getFurniture(FurnitureLayer::MIDDLE))
      c->getPosition().addFurniture(origin.getGame()->getContentFactory()->furniture.getFurniture(
          FurnitureType("SPIDER_WEB"), c->getTribeId()));
    for (auto& pos : Random.permutation(webPositions))
      if (auto victim = pos.getCreature())
        if (victim->isAffected(LastingEffect::ENTANGLED) && victim->isEnemy(c)) {
          attackPosition = pos;
          break;
        }
    if (!attackPosition)
      return c->moveTowards(origin);
    if (c->getPosition() == *attackPosition)
      return c->wait().append([this](Creature*) {
        attackPosition = none;
      });
    else
      return c->moveTowards(*attackPosition, NavigationFlags().requireStepOnTile());
  }

  virtual string getDescription() const override {
    return "Spider";
  }

  SERIALIZE_ALL(SUBCLASS(Task), origin, webPositions, attackPosition)
  SERIALIZATION_CONSTRUCTOR(Spider)

  protected:
  Position SERIAL(origin);
  vector<Position> SERIAL(webPositions);
  optional<Position> SERIAL(attackPosition);
};
}

PTask Task::spider(Position origin, const vector<Position>& posClose) {
  return makeOwner<Spider>(origin, posClose);
}

namespace {
class WithTeam : public Task {
  public:
  WithTeam(Collective* col, TeamId teamId, PTask task)
      : collective(std::move(col)), teamId(teamId), task(std::move(task)) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (task->isDone() || !collective->getTeams().exists(teamId)) {
      setDone();
      return NoMove;
    }
    auto leader = collective->getTeams().getLeader(teamId);
    CHECK(!leader->isDead());
    if (c == leader)
      return task->getMove(c);
    else {
      Position targetPos = leader->getPosition();
      if (targetPos.dist8(c->getPosition()).value_or(3) < 3) {
        if (Random.roll(15))
          if (auto move = c->move(c->getPosition().plus(Vec2(Random.choose<Dir>()))))
            return move;
        return NoMove;
      }
      return c->moveTowards(targetPos);
    }
  }

  virtual string getDescription() const override {
    return "Team task: " + task->getDescription();
  }

  SERIALIZE_ALL(SUBCLASS(Task), collective, teamId, task)
  SERIALIZATION_CONSTRUCTOR(WithTeam)

  private:
  Collective* SERIAL(collective) = nullptr;
  TeamId SERIAL(teamId);
  PTask SERIAL(task);
};
}

PTask Task::withTeam(Collective* col, TeamId teamId, PTask task) {
  return makeOwner<WithTeam>(std::move(col), teamId, std::move(task));
}

namespace {
class AllianceTask : public Task {
  public:
  AllianceTask(vector<Collective*> allies, Collective* enemy, PTask task)
      : allies(std::move(allies)), targetArea(getTargetArea(enemy)), task(std::move(task)) {}

  static PositionSet getTargetArea(Collective* col) {
    auto area = col->getTerritory().getExtended(10);
    return PositionSet(area.begin(), area.end());
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (targetArea.count(c->getPosition()))
      for (auto ally : allies)
        for (auto leader : ally->getLeaders())
          if (!leader->isDead() && !targetArea.count(leader->getPosition()))
            return c->wait();
    return task->getMove(c);
  }

  virtual string getDescription() const override {
    return "Team task: " + task->getDescription();
  }

  SERIALIZE_ALL(SUBCLASS(Task), allies, targetArea, task)
  SERIALIZATION_CONSTRUCTOR(AllianceTask)

  private:
  vector<Collective*> SERIAL(allies);
  PositionSet SERIAL(targetArea);
  PTask SERIAL(task);
};
}

PTask Task::allianceAttack(vector<Collective*> allies, Collective* enemy, PTask task) {
  return makeOwner<AllianceTask>(std::move(allies), enemy, std::move(task));
}

namespace {
class OutsidePredicate : public TaskPredicate {
  public:
  OutsidePredicate(Creature* c, PositionSet pos) : creature(c), positions(pos) {}

  virtual bool apply() const override {
    return !positions.count(creature->getPosition());
  }

  SERIALIZE_ALL(SUBCLASS(TaskPredicate), creature, positions)
  SERIALIZATION_CONSTRUCTOR(OutsidePredicate)

  private:
  Creature* SERIAL(creature) = nullptr;
  PositionSet SERIAL(positions);
};
}

TaskPredicate::~TaskPredicate() {}

PTaskPredicate TaskPredicate::outsidePositions(Creature* c, PositionSet pos) {
  return makeOwner<OutsidePredicate>(c, std::move(pos));
}

namespace {
class AlwaysPredicate : public TaskPredicate {
  public:
  AlwaysPredicate() {}

  virtual bool apply() const override {
    return true;
  }

  SERIALIZE_ALL(SUBCLASS(TaskPredicate))
};
}

PTaskPredicate TaskPredicate::always() {
  return makeOwner<AlwaysPredicate>();
}



template<class Archive>
void TaskPredicate::serialize(Archive& ar, const unsigned int version) {

}

REGISTER_TYPE(Construction)
REGISTER_TYPE(Destruction)
REGISTER_TYPE(PickUpItem)
REGISTER_TYPE(EquipItem)
REGISTER_TYPE(ApplySquare)
REGISTER_TYPE(Kill)
REGISTER_TYPE(Disappear)
REGISTER_TYPE(Chain)
REGISTER_TYPE(Explore)
REGISTER_TYPE(AbuseMinion)
REGISTER_TYPE(AttackCreatures)
REGISTER_TYPE(KillFighters)
REGISTER_TYPE(ConsumeItem)
REGISTER_TYPE(Copulate)
REGISTER_TYPE(Consume)
REGISTER_TYPE(Eat)
REGISTER_TYPE(GoTo)
REGISTER_TYPE(GoToAnd)
REGISTER_TYPE(Dance)
REGISTER_TYPE(StayIn)
REGISTER_TYPE(Idle)
REGISTER_TYPE(AlwaysDone)
REGISTER_TYPE(Follow)
REGISTER_TYPE(TransferTo)
REGISTER_TYPE(Whipping)
REGISTER_TYPE(GoToAndWait)
REGISTER_TYPE(DropItems)
REGISTER_TYPE(DropItemsAnywhere)
REGISTER_TYPE(CampAndSpawnTask)
REGISTER_TYPE(AllianceTask)
REGISTER_TYPE(Spider)
REGISTER_TYPE(WithTeam)
REGISTER_TYPE(ArcheryRange)
REGISTER_TYPE(OutsidePredicate)
REGISTER_TYPE(AlwaysPredicate)
