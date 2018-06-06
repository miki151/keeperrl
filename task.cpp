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

#include "task.h"
#include "level.h"
#include "entity_set.h"
#include "item.h"
#include "creature.h"
#include "collective.h"
#include "equipment.h"
#include "tribe.h"
#include "skill.h"
#include "creature_name.h"
#include "collective_teams.h"
#include "effect.h"
#include "creature_factory.h"
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

bool Task::canPerform(WConstCreature c) const {
  return true;
}

optional<Position> Task::getPosition() const {
  return none;
}

optional<ViewId> Task::getViewId() const {
  return viewId;
}

bool Task::isDone() {
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
    return "Build " + Furniture::getName(furnitureType) + " at " + toString(position);
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (!callback->isConstructionReachable(position))
      return NoMove;
    if (c->getPosition().dist8(position) > 1)
      return c->moveTowards(position);
    CHECK(c->getAttributes().getSkills().hasDiscrete(SkillId::CONSTRUCTION));
    Vec2 dir = c->getPosition().getDir(position);
    if (auto action = c->construct(dir, furnitureType))
      return {1.0, action.append([=](WCreature c) {
          if (!position.isActiveConstruction(Furniture::getLayer(furnitureType))) {
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
  WTaskCallback SERIAL(callback);
};

}

PTask Task::construction(WTaskCallback c, Position target, FurnitureType type) {
  return makeOwner<Construction>(c, target, type);
}

namespace {
class Destruction : public Task {
  public:
  Destruction(WTaskCallback c, Position pos, WConstFurniture furniture, DestroyAction action, WPositionMatching m)
      : Task(true), position(pos), callback(c), destroyAction(action),
        description(action.getVerbSecondPerson() + " "_s + furniture->getName() + " at " + toString(position)),
        furnitureType(furniture->getType()), matching(m) {
    if (matching)
      matching->addTarget(position);
  }

  WConstFurniture getFurniture() const {
    return position.getFurniture(Furniture::getLayer(furnitureType));
  }

  virtual bool isBogus() const override {
    if (auto furniture = getFurniture())
      if (furniture->canDestroy(destroyAction))
        return false;
    return true;
  }

  virtual bool canPerform(WConstCreature) const override {
    return callback->isConstructionReachable(position) && (!matching || matching->getMatch(position));
  }

  virtual string getDescription() const override {
    return description;
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (!canPerform(c))
      return NoMove;
    if (matching) {
      auto match = *matching->getMatch(position);
      if (c->getPosition() != match) {
        if (c->isSameSector(match))
          return c->moveTowards(*matching->getMatch(position));
      }
    }
    if (c->getPosition().dist8(position) > 1)
      return c->moveTowards(position);
    Vec2 dir = c->getPosition().getDir(position);
    CHECK(dir.length8() <= 1);
    if (auto action = c->destroy(dir, destroyAction))
      return {1.0, action.append([=](WCreature c) {
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

  ~Destruction() {
    if (matching)
      matching->releaseTarget(position);
  }

  SERIALIZE_ALL(SUBCLASS(Task), position, callback, destroyAction, description, furnitureType, matching)
  SERIALIZATION_CONSTRUCTOR(Destruction)

  private:
  Position SERIAL(position);
  WTaskCallback SERIAL(callback);
  DestroyAction SERIAL(destroyAction);
  string SERIAL(description);
  FurnitureType SERIAL(furnitureType);
  WPositionMatching SERIAL(matching);
};

}

PTask Task::destruction(WTaskCallback c, Position target, WConstFurniture furniture, DestroyAction destroyAction, WPositionMatching matching) {
  return makeOwner<Destruction>(c, target, furniture, destroyAction, matching);
}

PTask Task::bringItem(Position position, vector<WItem> items, const PositionSet& target) {
  return chain(Task::pickItem(position, items), dropItems(items, vector<Position>(target.begin(), target.end())));
}

namespace {

class PickItem : public Task {
  public:
  PickItem(Position pos, vector<WItem> _items, int retries = 10)
      : Task(true), items(_items), position(pos), tries(retries) {
    CHECK(!items.empty());
    lightestItem = 10000000;
    for (auto& item : _items)
      lightestItem = min(lightestItem, item->getWeight());
  }

  virtual void onPickedUp() {
    setDone();
  }

  virtual string getDescription() const override {
    return "Pick up item " + toString(position);
  }

  bool itemsExist(Position target) {
    for (WItem it : target.getItems())
      if (items.contains(it))
        return true;
    return false;
  }

  virtual MoveInfo getMove(WCreature c) override {
    CHECK(!pickedUp);
    if (!itemsExist(position)) {
      setDone();
      return NoMove;
    }
    if (c->getPosition() == position) {
      vector<WItem> hereItems;
      for (WItem it : c->getPickUpOptions())
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
        return {1.0, action.append([=](WCreature c) {
          pickedUp = true;
          onPickedUp();
        })}; 
      else {
        setDone();
        return NoMove;
      }
    }
    if (auto action = c->moveTowards(position, Creature::NavigationFlags().requireStepOnTile()))
      return action;
    else if (--tries == 0)
      setDone();
    return NoMove;
  }

  virtual bool canPerform(WConstCreature c) const override {
    return c->canCarryMoreWeight(lightestItem) && c->isSameSector(position);
  }

  SERIALIZE_ALL(SUBCLASS(Task), items, pickedUp, position, tries, lightestItem)
  SERIALIZATION_CONSTRUCTOR(PickItem)

  protected:
  EntitySet<Item> SERIAL(items);
  bool SERIAL(pickedUp) = false;
  Position SERIAL(position);
  int SERIAL(tries);
  double SERIAL(lightestItem);
};
}

PTask Task::pickItem(Position position, vector<WItem> items) {
  return makeOwner<PickItem>(position, items);
}

namespace {

class EquipItem : public Task {
  public:
  EquipItem(WItem item) : item(item->getUniqueId()), itemName(item->getName()) {
  }

  virtual string getDescription() const override {
    return "Equip " + itemName;
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (auto it = c->getEquipment().getItemById(item))
      if (auto action = c->equip(it))
        return {1.0, action.append([=](WCreature) {
          setDone();
        })};
    setDone();
    return NoMove;
  }

  SERIALIZE_ALL(item, itemName);
  SERIALIZATION_CONSTRUCTOR(EquipItem);

  private:
  UniqueEntity<Item>::Id SERIAL(item);
  string SERIAL(itemName);
};

}

PTask Task::pickAndEquipItem(Position position, WItem item) {
  return chain(pickItem(position, {item}), equipItem(item));
}

PTask Task::equipItem(WItem item) {
  return makeOwner<EquipItem>(item);
}

static optional<Position> chooseRandomClose(WCreature c, const vector<Position>& squares, Task::SearchType type) {
  int minD = 10000;
  int margin = type == Task::LAZY ? 0 : 3;
  vector<Position> close;
  auto start = c->getPosition();
  for (Position v : squares)
    if (c->canNavigateTo(v))
      minD = min(minD, v.dist8(start));
  for (Position v : squares)
    if (c->canNavigateTo(v) && v.dist8(start) <= minD + margin)
      close.push_back(v);
  if (!close.empty())
    return Random.choose(close);
  else {
    auto all = squares.filter([&](auto pos) { return c->canNavigateTo(pos); });
    if (!all.empty())
      return Random.choose(all);
    else
      return none;
  }
}

namespace {
class GoToAnd : public Task {
  public:
  GoToAnd(vector<Position> targets, PTask task) : targets(targets), task(std::move(task)) {}

  virtual bool canPerform(WConstCreature c) const override {
    if (!task->canPerform(c))
      return false;
    for (auto pos : targets)
      if (c->isSameSector(pos))
        return true;
    return false;
  }

  virtual string getDescription() const override {
    return task->getDescription();
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (task->isDone()) {
      setDone();
      return NoMove;
    }
    if (targets.contains(c->getPosition()))
      return task->getMove(c);
    if (!target)
      target = chooseRandomClose(c, targets, Task::SearchType::LAZY);
    if (!target) {
      setDone();
      return NoMove;
    } else
      return c->moveTowards(*target, Creature::NavigationFlags().requireStepOnTile());
  }

  SERIALIZATION_CONSTRUCTOR(GoToAnd)
  SERIALIZE_ALL(SUBCLASS(Task), target, targets, task)

  private:
  optional<Position> SERIAL(target);
  vector<Position> SERIAL(targets);
  PTask SERIAL(task);
};
}

class ApplyItem : public Task {
  public:
  ApplyItem(WTaskCallback c, WItem item) : callback(c), itemId(item->getUniqueId()), itemName(item->getShortName()) {}

  virtual string getDescription() const override {
    return "Set up " + itemName + " trap";
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (auto item = c->getEquipment().getItemById(itemId))
      if (auto action = c->applyItem(item))
        return action.prepend([=](WCreature c) {
          callback->onAppliedItem(c->getPosition(), item);
          setDone();
        });
    return c->wait().prepend([=](WCreature) {
       cancel();
    });
  }

  SERIALIZE_ALL(SUBCLASS(Task), callback, itemId, itemName);
  SERIALIZATION_CONSTRUCTOR(ApplyItem);

  private:
  WTaskCallback SERIAL(callback);
  Item::Id SERIAL(itemId);
  string SERIAL(itemName);
};

PTask Task::applyItem(WTaskCallback c, Position target, WItem item) {
  return makeOwner<GoToAnd>(vector<Position>{target}, makeOwner<ApplyItem>(c, item));
}

class ApplySquare : public Task {
  public:
  ApplySquare(WTaskCallback c, vector<Position> pos, SearchType t, ActionType a)
      : positions(pos), callback(c), searchType(t), actionType(a) {}

  void changePosIfOccupied() {
    if (position)
      if (WCreature c = position->getCreature())
        if (c->hasCondition(CreatureCondition::RESTRICTED_MOVEMENT))
          position = none;
  }

  optional<Position> choosePosition(WCreature c) {
    vector<Position> candidates;
    for (auto& pos : positions) {
      if (WCreature other = pos.getCreature())
        if (other->hasCondition(CreatureCondition::RESTRICTED_MOVEMENT))
          continue;
      if (!rejectedPosition.count(pos))
        candidates.push_back(pos);
    }
    return chooseRandomClose(c, candidates, searchType);
  }

  virtual MoveInfo getMove(WCreature c) override {
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
        return {1.0, action.append([=](WCreature c) {
            setDone();
            callback->onAppliedSquare(c, *position);
        })};
      else {
        setDone();
        return NoMove;
      }
    } else {
      MoveInfo move(c->moveTowards(*position));
      if (!move || (position->dist8(c->getPosition()) == 1 && position->getCreature() &&
          position->getCreature()->hasCondition(CreatureCondition::RESTRICTED_MOVEMENT))) {
        rejectedPosition.insert(*position);
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

  CreatureAction getAction(WCreature c) {
    switch (actionType) {
      case ActionType::APPLY:
        return c->applySquare(*position);
      case ActionType::NONE:
        return c->wait();
    }
  }

  virtual string getDescription() const override {
    return "Apply square " + (position ? toString(*position) : "");
  }

  bool atTarget(WCreature c) {
    return position == c->getPosition() || (!position->canEnterEmpty(c) && position->dist8(c->getPosition()) == 1);
  }

  SERIALIZE_ALL(SUBCLASS(Task), positions, rejectedPosition, invalidCount, position, callback, searchType, actionType)
  SERIALIZATION_CONSTRUCTOR(ApplySquare)

  private:
  vector<Position> SERIAL(positions);
  PositionSet SERIAL(rejectedPosition);
  int SERIAL(invalidCount) = 5;
  optional<Position> SERIAL(position);
  WTaskCallback SERIAL(callback);
  SearchType SERIAL(searchType);
  ActionType SERIAL(actionType);
};

PTask Task::applySquare(WTaskCallback c, vector<Position> position, SearchType searchType, ActionType actionType) {
  CHECK(position.size() > 0);
  return makeOwner<ApplySquare>(c, position, searchType, actionType);
}

namespace {

class ArcheryRange : public Task {
  public:
  ArcheryRange(WTaskCallback c, vector<Position> pos) : callback(c), targets(pos) {}

  virtual MoveInfo getMove(WCreature c) override {
    if (Random.roll(50))
      shootInfo = none;
    if (!shootInfo)
      shootInfo = getShootInfo(c);
    if (!shootInfo)
      return NoMove;
    if (c->getPosition() != shootInfo->pos)
      return c->moveTowards(shootInfo->pos, Creature::NavigationFlags().requireStepOnTile());
    if (Random.roll(3))
      return c->wait();
    for (auto pos = shootInfo->pos; pos != shootInfo->target; pos = pos.plus(shootInfo->dir)) {
      if (auto other = pos.plus(shootInfo->dir).getCreature())
        if (other->isFriend(c))
          return c->wait();
    }
    if (auto move = c->fire(shootInfo->dir))
      return move.append(
          [this, target = shootInfo->target](WCreature c) {
            callback->onAppliedSquare(c, target);
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
  WTaskCallback SERIAL(callback);
  vector<Position> SERIAL(targets);
  struct ShootInfo {
    Position SERIAL(pos);
    Position SERIAL(target);
    Vec2 SERIAL(dir);
    SERIALIZE_ALL(pos, target, dir)
  };
  optional<ShootInfo> SERIAL(shootInfo);

  optional<ShootInfo> getShootInfo(WCreature c) {
    const int distance = 5;
    auto getDir = [&](Position target) -> optional<ShootInfo> {
      for (Vec2 dir : Vec2::directions4(Random)) {
        bool ok = true;
        for (int i : Range(distance))
          if (target.minus(dir * (i + 1)).stopsProjectiles(c->getVision().getId())) {
            ok = false;
            break;
          }
        auto pos = target.minus(dir * distance);
        if (ok && c->isSameSector(pos))
          return ShootInfo{pos, target, dir};
      }
      return none;
    };
    unordered_map<Position, vector<ShootInfo>, CustomHash<Position>> shootPositions;
    for (auto pos : Random.permutation(targets))
      if (auto dir = getDir(pos))
        shootPositions[dir->pos].push_back(*dir);
    if (auto chosen = chooseRandomClose(c, getKeys(shootPositions), Task::RANDOM_CLOSE))
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
  enum Type { ATTACK, TORTURE };
  Kill(WTaskCallback call, WCreature c, Type t) : creature(c), type(t), callback(call) {}

  CreatureAction getAction(WCreature c) {
    switch (type) {
      case ATTACK: return c->execute(creature);
      case TORTURE: return c->torture(creature);
    }
  }

  virtual string getDescription() const override {
    switch (type) {
      case ATTACK: return "Kill " + creature->getName().bare();
      case TORTURE: return "Torture " + creature->getName().bare();
    }
  }

  virtual bool canTransfer() override {
    return true;
  }

  virtual bool canPerform(WConstCreature c) const override {
    return c != creature;
  }

  virtual MoveInfo getMove(WCreature c) override {
    CHECK(c != creature);
    if (creature->isDead() || (type == TORTURE && !creature->isAffected(LastingEffect::TIED_UP))) {
      setDone();
      return NoMove;
    }
    if (auto action = getAction(c))
      return action.append([=](WCreature c) { if (creature->isDead()) setDone(); });
    else
      return c->moveTowards(creature->getPosition());
  }

  SERIALIZE_ALL(SUBCLASS(Task), creature, type, callback); 
  SERIALIZATION_CONSTRUCTOR(Kill);

  private:
  WCreature SERIAL(creature);
  Type SERIAL(type);
  WTaskCallback SERIAL(callback);
};

}

PTask Task::kill(WTaskCallback callback, WCreature creature) {
  return makeOwner<Kill>(callback, creature, Kill::ATTACK);
}

PTask Task::torture(WTaskCallback callback, WCreature creature) {
  return makeOwner<Kill>(callback, creature, Kill::TORTURE);
}

namespace {

class Disappear : public Task {
  public:
  Disappear() {}

  virtual MoveInfo getMove(WCreature c) override {
    return c->disappear();
  }

  virtual string getDescription() const override {
    return "Disappear";
  }

  SERIALIZE_ALL(SUBCLASS(Task)); 
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

  virtual bool canTransfer() override {
    return tasks[current]->canTransfer();
  }

  virtual void cancel() override {
    for (int i = current; i < tasks.size(); ++i)
      if (!tasks[i]->isDone())
        tasks[i]->cancel();
  }

  virtual MoveInfo getMove(WCreature c) override {
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

  SERIALIZE_ALL(SUBCLASS(Task), tasks, current); 
  SERIALIZATION_CONSTRUCTOR(Chain);

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

  virtual MoveInfo getMove(WCreature c) override {
    if (!Random.roll(3))
      return NoMove;
    if (auto action = c->moveTowards(position))
      return action.append([=](WCreature c) {
          if (c->getPosition().dist8(position) < 5)
            setDone();
      });
    if (Random.roll(3))
      setDone();
    return NoMove;
  }

  virtual string getDescription() const override {
    return "Explore " + toString(position);
  }

  SERIALIZE_ALL(SUBCLASS(Task), position); 
  SERIALIZATION_CONSTRUCTOR(Explore);

  private:
  Position SERIAL(position);
};

}

PTask Task::explore(Position pos) {
  return makeOwner<Explore>(pos);
}

namespace {

class AttackCreatures : public Task {
  public:
  AttackCreatures(vector<WCreature> v) : creatures(v) {}

  virtual MoveInfo getMove(WCreature c) override {
    if (auto target = getNextCreature())
      return c->moveTowards(target->getPosition());
    return NoMove;
  }

  WCreature getNextCreature() const {
    for (auto c : creatures)
      if (c && !c->isDead())
        return c;
    return nullptr;
  }

  virtual string getDescription() const override {
    if (auto target = getNextCreature())
      return "Attack " + target->getName().bare();
    else
      return "Attack someone, but everyone is dead";
  }
  
  SERIALIZE_ALL(SUBCLASS(Task), creatures);
  SERIALIZATION_CONSTRUCTOR(AttackCreatures);

  private:
  vector<WCreature> SERIAL(creatures);
};

}

PTask Task::attackCreatures(vector<WCreature> c) {
  return makeOwner<AttackCreatures>(std::move(c));
}

PTask Task::stealFrom(WCollective collective) {
  vector<PTask> tasks;
  for (Position pos : collective->getConstructions().getBuiltPositions(FurnitureType::TREASURE_CHEST)) {
    vector<WItem> gold = pos.getItems().filter(Item::classPredicate(ItemClass::GOLD));
    if (!gold.empty())
      tasks.push_back(pickItem(pos, gold));
  }
  if (!tasks.empty())
    return chain(std::move(tasks));
  else
    return PTask(nullptr);
}

namespace {

class CampAndSpawn : public Task {
  public:
  CampAndSpawn(WCollective _target, CreatureFactory s, int defense, Range attack, int numAtt)
    : target(_target), spawns(s),
      campPos(Random.permutation(target->getTerritory().getStandardExtended())), defenseSize(defense),
      attackSize(attack), numAttacks(numAtt) {}

  void updateTeams() {
    for (auto member : copyOf(defenseTeam))
      if (member->isDead())
        defenseTeam.removeElement(member);
    for (auto member : copyOf(attackTeam))
      if (member->isDead())
        attackTeam.removeElement(member);
  }

  virtual void cancel() override {
    // Cancel only the attack team, as the defense will disappear when the summoner dies
    for (WCreature c : attackTeam)
      if (!c->isDead())
        c->dieNoReason(Creature::DropType::NOTHING);
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (!target->getLeader() || campPos.empty()) {
      setDone();
      return NoMove;
    }
    updateTeams();
    if (defenseTeam.size() < defenseSize && Random.roll(5)) {
      for (WCreature summon : Effect::summonCreatures(c, 4,
          makeVec(spawns.random(MonsterAIFactory::summoned(c)))))
        defenseTeam.push_back(summon);
    }
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
        vector<PCreature> team;
        for (int i : Range(Random.get(attackSize)))
          team.push_back(spawns.random(MonsterAIFactory::singleTask(Task::attackCreatures({target->getLeader()}))));
        for (WCreature summon : Effect::summonCreatures(c, 4, std::move(team)))
          attackTeam.push_back(summon);
        attackCountdown = none;
      }
    }
    return c->wait();
  }

  virtual string getDescription() const override {
    return "Camp and spawn " + target->getLeader()->getName().bare();
  }
 
  SERIALIZE_ALL(SUBCLASS(Task), target, spawns, campPos, defenseSize, attackSize, attackCountdown, defenseTeam, attackTeam, numAttacks);
  SERIALIZATION_CONSTRUCTOR(CampAndSpawn);

  private:
  WCollective SERIAL(target);
  CreatureFactory SERIAL(spawns);
  vector<Position> SERIAL(campPos);
  int SERIAL(defenseSize);
  Range SERIAL(attackSize);
  optional<int> SERIAL(attackCountdown);
  vector<WCreature> SERIAL(defenseTeam);
  vector<WCreature> SERIAL(attackTeam);
  int SERIAL(numAttacks);
};


}

PTask Task::campAndSpawn(WCollective target, const CreatureFactory& spawns, int defenseSize,
    Range attackSize, int numAttacks) {
  return makeOwner<CampAndSpawn>(target, spawns, defenseSize, attackSize, numAttacks);
}

namespace {

class KillFighters : public Task {
  public:
  KillFighters(WCollective col, int numC) : collective(col), numCreatures(numC) {}

  virtual MoveInfo getMove(WCreature c) override {
    for (WConstCreature target : collective->getCreatures(MinionTrait::FIGHTER))
      targets.insert(target);
    int numKilled = 0;
    for (Creature::Id victim : c->getKills())
      if (targets.contains(victim))
        ++numKilled;
    if (numKilled >= numCreatures || targets.empty()) {
      setDone();
      return NoMove;
    }
    return c->moveTowards(collective->getLeader()->getPosition());
  }

  virtual string getDescription() const override {
    return "Kill " + toString(numCreatures) + " minions of " + collective->getName()->full;
  }

  SERIALIZE_ALL(SUBCLASS(Task), collective, numCreatures, targets); 
  SERIALIZATION_CONSTRUCTOR(KillFighters);

  private:
  WCollective SERIAL(collective);
  int SERIAL(numCreatures);
  EntitySet<Creature> SERIAL(targets);
};

}

PTask Task::killFighters(WCollective col, int numCreatures) {
  return makeOwner<KillFighters>(col, numCreatures);
}

namespace {
class ConsumeItem : public Task {
  public:
  ConsumeItem(WTaskCallback c, vector<WItem> _items) : items(_items), callback(c) {}

  virtual MoveInfo getMove(WCreature c) override {
    return c->wait().append([=](WCreature c) {
        c->getEquipment().removeItems(c->getEquipment().getItems().filter(items.containsPredicate()), c); });
  }

  virtual string getDescription() const override {
    return "Consume item";
  }
  
  SERIALIZE_ALL(SUBCLASS(Task), items, callback); 
  SERIALIZATION_CONSTRUCTOR(ConsumeItem);

  protected:
  EntitySet<Item> SERIAL(items);
  WTaskCallback SERIAL(callback);
};
}

PTask Task::consumeItem(WTaskCallback c, vector<WItem> items) {
  return makeOwner<ConsumeItem>(c, items);
}

namespace {
class Copulate : public Task {
  public:
  Copulate(WTaskCallback c, WCreature t, int turns) : target(t), callback(c), numTurns(turns) {}

  virtual MoveInfo getMove(WCreature c) override {
    if (target->isDead() || !target->isAffected(LastingEffect::SLEEP)) {
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
        return action.append([=](WCreature c) {
          if (--numTurns == 0) {
            setDone();
            callback->onCopulated(c, target);
          }});
      else
        return NoMove;
    } else
      return c->moveTowards(target->getPosition());
  }

  virtual string getDescription() const override {
    return "Copulate with " + target->getName().bare();
  }

  SERIALIZE_ALL(SUBCLASS(Task), target, numTurns, callback); 
  SERIALIZATION_CONSTRUCTOR(Copulate);

  protected:
  WCreature SERIAL(target);
  WTaskCallback SERIAL(callback);
  int SERIAL(numTurns);
};
}

PTask Task::copulate(WTaskCallback c, WCreature target, int numTurns) {
  return makeOwner<Copulate>(c, target, numTurns);
}

namespace {
class Consume : public Task {
  public:
  Consume(WCreature t) : target(t) {}

  virtual MoveInfo getMove(WCreature c) override {
    if (target->isDead()) {
      setDone();
      return NoMove;
    }
    if (c->getPosition().dist8(target->getPosition()) == 1) {
      if (auto action = c->consume(target))
        return action.append([=](WCreature c) {
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

  SERIALIZE_ALL(SUBCLASS(Task), target);
  SERIALIZATION_CONSTRUCTOR(Consume);

  protected:
  WCreature SERIAL(target);
};
}

PTask Task::consume(WCreature target) {
  return makeOwner<Consume>(target);
}

namespace {

class Eat : public Task {
  public:
  Eat(vector<Position> pos) : positions(pos) {}

  virtual bool canTransfer() override {
    return false;
  }

  WItem getDeadChicken(Position pos) {
    vector<WItem> chickens = pos.getItems().filter(Item::classPredicate(ItemClass::FOOD));
    if (chickens.empty())
      return nullptr;
    else
      return chickens[0];
  }

  virtual MoveInfo getMove(WCreature c) override {
    PROFILE;
    if (!position) {
      for (Position v : Random.permutation(positions))
        if (!rejectedPosition.count(v) && (!position ||
              position->dist8(c->getPosition()) > v.dist8(c->getPosition())))
          position = v;
      if (!position) {
        setDone();
        return NoMove;
      }
    }
    if (c->getPosition() != *position && getDeadChicken(*position))
      return c->moveTowards(*position);
    WItem chicken = getDeadChicken(c->getPosition());
    if (chicken)
      return c->eat(chicken).append([=] (WCreature c) {
        setDone();
      });
    for (Position pos : c->getPosition().neighbors8(Random)) {
      WItem chicken = getDeadChicken(pos);
      if (chicken) 
        if (auto move = c->move(pos))
          return move;
      if (WCreature ch = pos.getCreature())
        if (ch->getBody().isMinionFood())
          if (auto move = c->attack(ch)) {
            return move.append([this, ch, pos] (WCreature) { if (ch->isDead()) position = pos; });
      }
    }
    for (auto chicken : c->getVisibleCreatures())
      if (chicken->getBody().isMinionFood())
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

  SERIALIZE_ALL(SUBCLASS(Task), position, positions, rejectedPosition); 
  SERIALIZATION_CONSTRUCTOR(Eat);

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

  virtual MoveInfo getMove(WCreature c) override {
    if (c->getPosition() == target ||
        (c->getPosition().dist8(target) == 1 && !target.canEnter(c)) ||
        (!tryForever && !c->canNavigateTo(target))) {
      setDone();
      return NoMove;
    } else
      return c->moveTowards(target);
  }

  virtual string getDescription() const override {
    return "Go to " + toString(target);
  }

  SERIALIZE_ALL(SUBCLASS(Task), target, tryForever)
  SERIALIZATION_CONSTRUCTOR(GoTo);

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
class StayIn : public Task {
  public:
  StayIn(vector<Position> pos) : target(std::move(pos)) {}

  virtual MoveInfo getMove(WCreature c) override {
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
    if (currentTarget)
      return c->moveTowards(*currentTarget);
    else
      return c->wait();
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
  return makeOwner<StayIn>(pos);
}

namespace {
class Idle : public Task {
  public:

  virtual MoveInfo getMove(WCreature c) override {
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

  virtual MoveInfo getMove(WCreature c) override {
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

  virtual bool canPerform(WConstCreature c) const override {
    return task->canPerform(c);
  }

  virtual optional<Position> getPosition() const override {
    return task->getPosition();
  }

  SERIALIZE_ALL(SUBCLASS(Task), task, donePredicate)
  SERIALIZATION_CONSTRUCTOR(AlwaysDone);

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
  Follow(WCreature t) : target(t) {}

  virtual MoveInfo getMove(WCreature c) override {
    if (!target->isDead()) {
      Position targetPos = target->getPosition();
      if (targetPos.dist8(c->getPosition()) < 3) {
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

  WCreature SERIAL(target);

  SERIALIZE_ALL(SUBCLASS(Task), target)
  SERIALIZATION_CONSTRUCTOR(Follow);
};
}

PTask Task::follow(WCreature c) {
  return makeOwner<Follow>(c);
}

namespace {
class TransferTo : public Task {
  public:
  TransferTo(WModel m) : model(m) {}

  virtual MoveInfo getMove(WCreature c) override {
    if (c->getPosition().getModel() == model) {
      setDone();
      return NoMove;
    }
    if (!target)
      target = c->getGame()->getTransferPos(model, c->getPosition().getModel());
    if (c->getPosition() == target && c->getGame()->canTransferCreature(c, model)) {
      return c->wait().append([=] (WCreature c) { setDone(); c->getGame()->transferCreature(c, model); });
    } else
      return c->moveTowards(*target);
  }

  virtual string getDescription() const override {
    return "Go to site";
  }

  SERIALIZE_ALL(SUBCLASS(Task), target, model); 
  SERIALIZATION_CONSTRUCTOR(TransferTo);

  protected:
  optional<Position> SERIAL(target);
  WModel SERIAL(model);
};
}

PTask Task::transferTo(WModel m) {
  return makeOwner<TransferTo>(m);
}

namespace {
class GoToAndWait : public Task {
  public:
  GoToAndWait(Position pos, TimeInterval waitT) : position(pos), waitTime(waitT) {}

  bool arrived(WCreature c) {
    return c->getPosition() == position ||
        (c->getPosition().dist8(position) == 1 && !position.canEnterEmpty(c));
  }

  virtual optional<Position> getPosition() const override {
    return position;
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (maxTime && c->getLocalTime() >= *maxTime) {
      setDone();
      return NoMove;
    }
    if (!arrived(c)) {
      auto ret = c->moveTowards(position);
      if (!ret) {
        if (!timeout)
          timeout = *c->getLocalTime() + 30_visible;
        else
          if (c->getLocalTime() > *timeout) {
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

  SERIALIZE_ALL(SUBCLASS(Task), position, waitTime, maxTime, timeout);
  SERIALIZATION_CONSTRUCTOR(GoToAndWait);

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
  Whipping(Position pos, WCreature w) : position(pos), whipped(w) {}

  virtual bool canPerform(WConstCreature c) const override {
    return c != whipped;
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (position.getCreature() != whipped || !whipped->isAffected(LastingEffect::TIED_UP)) {
      setDone();
      return NoMove;
    }
    if (c->getPosition().dist8(position) > 1)
      return c->moveTowards(position);
    else
      return c->whip(whipped->getPosition());
  }

  virtual string getDescription() const override {
    return "Whipping " + whipped->getName().a();
  }

  SERIALIZE_ALL(SUBCLASS(Task), position, whipped);
  SERIALIZATION_CONSTRUCTOR(Whipping);

  protected:
  Position SERIAL(position);
  WCreature SERIAL(whipped);
};
}

PTask Task::whipping(Position pos, WCreature whipped) {
  return makeOwner<Whipping>(pos, whipped);
}

namespace {
class DropItemsAnywhere : public Task {
  public:
  DropItemsAnywhere(EntitySet<Item> it) : items(it) {}

  virtual MoveInfo getMove(WCreature c) override {
    return c->drop(c->getEquipment().getItems().filter(items.containsPredicate())).append([=] (WCreature) { setDone(); });
  }

  virtual string getDescription() const override {
    return "Drop items anywhere";
  }

  SERIALIZE_ALL(SUBCLASS(Task), items); 
  SERIALIZATION_CONSTRUCTOR(DropItemsAnywhere);

  protected:
  EntitySet<Item> SERIAL(items);
};
}

PTask Task::dropItemsAnywhere(vector<WItem> items) {
  return makeOwner<DropItemsAnywhere>(items);
}

namespace {
class DropItems : public Task {
  public:
  DropItems(EntitySet<Item> it, vector<Position> pos) : items(it), positions(pos) {}


  virtual string getDescription() const override {
    return "Drop items";
  }

  optional<Position> getBestTarget(WCreature c, const vector<Position>& pos) const {
    return chooseRandomClose(c, pos, LAZY);
  }

  virtual MoveInfo getMove(WCreature c) override {
    if (!target || !c->isSameSector(*target))
      target = getBestTarget(c, positions);
    if (!target)
      return c->drop(c->getEquipment().getItems().filter(items.containsPredicate())).append(
          [this] (WCreature) {
            setDone();
          });
    if (c->getPosition() == target) {
      vector<WItem> myItems = c->getEquipment().getItems().filter(items.containsPredicate());
      if (auto action = c->drop(myItems).append([=] (WCreature) { setDone(); }))
        return {1.0, action.append([=](WCreature) {setDone();})};
      else {
        setDone();
        return NoMove;
      }
    } else {
      if (c->getPosition().dist8(*target) == 1)
        if (WCreature other = target->getCreature())
          if (other->isAffected(LastingEffect::SLEEP))
            other->removeEffect(LastingEffect::SLEEP);
      return c->moveTowards(*target);
    }
  }

  SERIALIZE_ALL(SUBCLASS(Task), items, positions, target);
  SERIALIZATION_CONSTRUCTOR(DropItems);

  protected:
  EntitySet<Item> SERIAL(items);
  vector<Position> SERIAL(positions);
  optional<Position> SERIAL(target);
};
}

PTask Task::dropItems(vector<WItem> items, vector<Position> positions) {
  return makeOwner<DropItems>(items, positions);
}

namespace {
class Spider : public Task {
  public:
  Spider(Position orig, const vector<Position>& pos)
      : origin(orig), webPositions(pos) {}

  virtual MoveInfo getMove(WCreature c) override {
    auto layer = Furniture::getLayer(FurnitureType::SPIDER_WEB);
    for (auto pos : webPositions)
      if (!pos.getFurniture(layer))
        pos.addFurniture(FurnitureFactory::get(FurnitureType::SPIDER_WEB, c->getTribeId()));
    for (auto& pos : Random.permutation(webPositions))
      if (pos.getCreature() && pos.getCreature()->isAffected(LastingEffect::ENTANGLED)) {
        attackPosition = pos;
        break;
      }
    if (!attackPosition)
      return c->moveTowards(origin);
    if (c->getPosition() == *attackPosition)
      return c->wait().append([this](WCreature) {
        attackPosition = none;
      });
    else
      return c->moveTowards(*attackPosition, Creature::NavigationFlags().requireStepOnTile());
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
class OutsidePredicate : public TaskPredicate {
  public:
  OutsidePredicate(WCreature c, PositionSet pos) : creature(c), positions(pos) {}

  virtual bool apply() const override {
    return !positions.count(creature->getPosition());
  }

  SERIALIZE_ALL(SUBCLASS(TaskPredicate), creature, positions)
  SERIALIZATION_CONSTRUCTOR(OutsidePredicate)

  private:
  WCreature SERIAL(creature);
  PositionSet SERIAL(positions);
};
}

PTaskPredicate TaskPredicate::outsidePositions(WCreature c, PositionSet pos) {
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
REGISTER_TYPE(PickItem)
REGISTER_TYPE(EquipItem)
REGISTER_TYPE(ApplyItem)
REGISTER_TYPE(ApplySquare)
REGISTER_TYPE(Kill)
REGISTER_TYPE(Disappear)
REGISTER_TYPE(Chain)
REGISTER_TYPE(Explore)
REGISTER_TYPE(AttackCreatures)
REGISTER_TYPE(KillFighters)
REGISTER_TYPE(ConsumeItem)
REGISTER_TYPE(Copulate)
REGISTER_TYPE(Consume)
REGISTER_TYPE(Eat)
REGISTER_TYPE(GoTo)
REGISTER_TYPE(GoToAnd)
REGISTER_TYPE(StayIn)
REGISTER_TYPE(Idle)
REGISTER_TYPE(AlwaysDone)
REGISTER_TYPE(Follow)
REGISTER_TYPE(TransferTo)
REGISTER_TYPE(Whipping)
REGISTER_TYPE(GoToAndWait)
REGISTER_TYPE(DropItems)
REGISTER_TYPE(DropItemsAnywhere)
REGISTER_TYPE(CampAndSpawn)
REGISTER_TYPE(Spider)
REGISTER_TYPE(ArcheryRange)
REGISTER_TYPE(OutsidePredicate)
REGISTER_TYPE(AlwaysPredicate)
