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
#include "square.h"

template <class Archive> 
void Task::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(UniqueEntity)
    & SVAR(done)
    & SVAR(callback);
  CHECK_SERIAL;
}

SERIALIZABLE(Task);
SERIALIZATION_CONSTRUCTOR_IMPL(Task);

template <class Archive> 
void Task::Callback::serialize(Archive& ar, const unsigned int version) {
}

SERIALIZABLE(Task::Callback);
SERIALIZATION_CONSTRUCTOR_IMPL2(Task, Callback);

template <class Archive>

void Task::Mapping::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(tasks)
    & BOOST_SERIALIZATION_NVP(positionMap)
    & BOOST_SERIALIZATION_NVP(creatureMap);
}

SERIALIZABLE(Task::Mapping);

Task::Task(Callback* call) : callback(call) {}

Task::~Task() {
}

Task::Callback* Task::getCallback() {
  return callback;
}

bool Task::isImpossible(const Level*) {
  return false;
}

bool Task::canTransfer() {
  return true;
}

bool Task::isDone() {
  return done;
}

void Task::setDone() {
  done = true;
}

void Task::Mapping::removeTask(Task* task) {
  for (int i : All(tasks))
    if (tasks[i].get() == task) {
      removeIndex(tasks, i);
      break;
    }
  if (creatureMap.contains(task))
    creatureMap.erase(task);
  if (positionMap.count(task))
    positionMap.erase(task);
}

void Task::Mapping::removeTask(UniqueId id) {
  for (PTask& task : tasks)
    if (task->getUniqueId() == id) {
      removeTask(task.get());
      break;
    }
}

Task* Task::Mapping::getTask(const Creature* c) const {
  if (creatureMap.contains(c))
    return creatureMap.get(c);
  else
    return nullptr;
}

Task* Task::Mapping::addTask(PTask task, const Creature* c) {
  creatureMap.insert(c, task.get());
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

Task* Task::Mapping::addTask(PTask task, Vec2 position) {
  positionMap[task.get()] = position;
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

void Task::Mapping::takeTask(const Creature* c, Task* task) {
  freeTask(task);
  creatureMap.insert(c, task);
}

Optional<Vec2> Task::Mapping::getPosition(Task* task) const {
  if (positionMap.count(task))
    return positionMap.at(task);
  else
    return Nothing();
}

const Creature* Task::Mapping::getOwner(Task* task) const {
  if (creatureMap.contains(task))
    return creatureMap.get(task);
  else
    return nullptr;
}

void Task::Mapping::freeTask(Task* task) {
  if (creatureMap.contains(task))
    creatureMap.erase(task);
}

class Construction : public Task {
  public:
  Construction(Callback* col, Vec2 pos, SquareType _type) : Task(col), type(_type), position(pos) {}

  virtual bool isImpossible(const Level* level) {
    return !level->getSquare(position)->canConstruct(type);
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!c->hasSkill(Skill::get(SkillId::CONSTRUCTION)))
      return NoMove;
    Vec2 dir = position - c->getPosition();
    if (dir.length8() == 1) {
      if (auto action = c->construct(dir, type))
        return {1.0, action.append([=] {
          if (!c->construct(dir, type)) {
            setDone();
            getCallback()->onConstructed(position, type);
          }
        })};
      else {
        setDone();
        return NoMove;
      }
    } else
        return c->moveTowards(position);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Task)
      & SVAR(position)
      & SVAR(type);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Construction);

  private:
  SquareType SERIAL(type);
  Vec2 SERIAL(position);
};


PTask Task::construction(Callback* col, Vec2 target, SquareType type) {
  return PTask(new Construction(col, target, type));
}
  
class PickItem : public Task {
  public:
  PickItem(Callback* col, Vec2 pos, vector<Item*> _items) : Task(col), items(_items), position(pos) {}

  virtual void onPickedUp() {
    setDone();
  }

  virtual bool canTransfer() override {
    return false;
  }

  Vec2 getPosition() {
    return position;
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
      getCallback()->onCantPickItem(items);
      if (hereItems.empty()) {
        setDone();
        return NoMove;
      }
      items = hereItems;
      if (auto action = c->pickUp(hereItems))
        return {1.0, action.append([=] {
          pickedUp = true;
          onPickedUp();
          getCallback()->onPickedUp(position, hereItems);
        })}; 
      else {
        getCallback()->onCantPickItem(items);
        setDone();
        return NoMove;
      }
    }
    if (auto action = c->moveTowards(position, true))
      return action;
    else if (--tries == 0) {
      getCallback()->onCantPickItem(items);
      setDone();
    }
    return NoMove;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Task)
      & SVAR(items)
      & SVAR(pickedUp)
      & SVAR(position)
      & SVAR(tries);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(PickItem);

  protected:
  EntitySet SERIAL(items);
  bool SERIAL2(pickedUp, false);
  Vec2 SERIAL(position);
  int SERIAL2(tries, 10);
};

PTask Task::pickItem(Callback* col, Vec2 position, vector<Item*> items) {
  return PTask(new PickItem(col, position, items));
}

class EquipItem : public PickItem {
  public:
  EquipItem(Callback* col, Vec2 position, vector<Item*> _items) : PickItem(col, position, _items) {
  }

  virtual void onPickedUp() override {
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!pickedUp)
      return PickItem::getMove(c);
    vector<Item*> it = c->getEquipment().getItems(items.containsPredicate());
    if (!it.empty())
      if (auto action = c->equip(getOnlyElement(it)))
        return {1.0, action.append([=] {
          setDone();
        })};
    setDone();
    return NoMove;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(PickItem);
  }
  
  SERIALIZATION_CONSTRUCTOR(EquipItem);
};

PTask Task::equipItem(Callback* col, Vec2 position, Item* items) {
  return PTask(new EquipItem(col, position, {items}));
}

static Vec2 chooseRandomClose(Vec2 start, const vector<Vec2>& squares) {
  int minD = 10000;
  int margin = 5;
  int a;
  vector<Vec2> close;
  for (Vec2 v : squares)
    if ((a = v.dist8(start)) < minD)
      minD = a;
  for (Vec2 v : squares)
    if (v.dist8(start) < minD + margin)
      close.push_back(v);
  CHECK(!close.empty());
  return chooseRandom(close);
}

class BringItem : public PickItem {
  public:
  BringItem(Callback* col, Vec2 position, vector<Item*> items, vector<Vec2> _target)
      : PickItem(col, position, items), target(chooseRandomClose(position, _target)) {}

  virtual CreatureAction getBroughtAction(Creature* c, vector<Item*> it) {
    return c->drop(it);
  }

  virtual void onPickedUp() override {
  }
  
  virtual MoveInfo getMove(Creature* c) override {
    if (!pickedUp)
      return PickItem::getMove(c);
    if (c->getPosition() == target) {
      vector<Item*> myItems = c->getEquipment().getItems(items.containsPredicate());
      if (auto action = getBroughtAction(c, myItems))
        return {1.0, action.append([=] {setDone();})};
      else {
        setDone();
        return NoMove;
      }
    } else {
      if (c->getPosition().dist8(target) == 1)
        if (Creature* other = c->getLevel()->getSquare(target)->getCreature())
          if (other->isAffected(LastingEffect::SLEEP))
            other->removeEffect(LastingEffect::SLEEP);
      return c->moveTowards(target);
    }
  }

  virtual bool canTransfer() override {
    return !pickedUp;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(PickItem)
      & SVAR(target);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(BringItem);

  protected:
  Vec2 SERIAL(target);
};

PTask Task::bringItem(Callback* col, Vec2 position, vector<Item*> items, vector<Vec2> target) {
  return PTask(new BringItem(col, position, items, target));
}

class ApplyItem : public BringItem {
  public:
  ApplyItem(Callback* col, Vec2 position, vector<Item*> items, Vec2 _target) 
      : BringItem(col, position, items, {_target}) {}

  virtual void cancel() override {
    getCallback()->onAppliedItemCancel(target);
  }

  virtual CreatureAction getBroughtAction(Creature* c, vector<Item*> it) override {
    Item* item = getOnlyElement(it);
    return c->applyItem(item).prepend([=] {
        getCallback()->onAppliedItem(c->getPosition(), item);
    });
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(BringItem);
  }
  
  SERIALIZATION_CONSTRUCTOR(ApplyItem);
};

PTask Task::applyItem(Callback* col, Vec2 position, Item* item, Vec2 target) {
  return PTask(new ApplyItem(col, position, {item}, target));
}

class ApplySquare : public Task {
  public:
  ApplySquare(Callback* col, vector<Vec2> pos) : Task(col), positions(pos) {}

  virtual bool canTransfer() override {
    return false;
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (position.x == -1) {
      vector<Vec2> candidates = filter(positions, [&](Vec2 v) { return !rejectedPosition.count(v); });
      if (!candidates.empty())
        position = chooseRandomClose(c->getPosition(), candidates);
      else {
        setDone();
        return NoMove;
      }
    }
    if (c->getPosition() == position) {
      if (auto action = c->applySquare())
        return {1.0, action.append([=] {
            setDone();
            getCallback()->onAppliedSquare(c->getPosition());
        })};
      else {
        setDone();
        return NoMove;
      }
    } else {
      MoveInfo move(c->moveTowards(position));
      if (!move || (position.dist8(c->getPosition()) == 1
            && c->getLevel()->getSquare(position)->getCreature())) {
        rejectedPosition.insert(position);
        position = Vec2(-1, -1);
        if (--invalidCount == 0) {
          setDone();
        }
        return NoMove;
      }
      else 
        return move;
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Task)
      & SVAR(positions)
      & SVAR(rejectedPosition)
      & SVAR(invalidCount)
      & SVAR(position)
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(ApplySquare);

  private:
  vector<Vec2> SERIAL(positions);
  set<Vec2> SERIAL(rejectedPosition);
  int SERIAL2(invalidCount, 5);
  Vec2 SERIAL2(position, Vec2(-1, -1));
};

PTask Task::applySquare(Callback* col, vector<Vec2> position) {
  CHECK(position.size() > 0);
  return PTask(new ApplySquare(col, position));
}

namespace {

class Kill : public Task {
  public:
  enum Type { ATTACK, TORTURE };
  Kill(Callback* callback, Creature* c, Type t) : Task(callback), creature(c), type(t) {}

  CreatureAction getAction(Creature* c) {
    switch (type) {
      case ATTACK: return c->attack(creature);
      case TORTURE: return c->torture(creature);
    }
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (auto action = getAction(c))
      return action.append([=] { if (creature->isDead()) setDone(); });
    else
      return c->moveTowards(creature->getPosition());
  }

  virtual void cancel() override {
    getCallback()->onKillCancelled(creature);
  }

  virtual bool canTransfer() override {
    return false;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Task)
      & SVAR(creature);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Kill);

  private:
  Creature* SERIAL(creature);
  Type SERIAL(type);
};

}

PTask Task::kill(Callback* callback, Creature* creature) {
  return PTask(new Kill(callback, creature, Kill::ATTACK));
}

PTask Task::torture(Callback* callback, Creature* creature) {
  return PTask(new Kill(callback, creature, Kill::TORTURE));
}

template <class Archive>
void Task::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, Construction);
  REGISTER_TYPE(ar, PickItem);
  REGISTER_TYPE(ar, EquipItem);
  REGISTER_TYPE(ar, BringItem);
  REGISTER_TYPE(ar, ApplyItem);
  REGISTER_TYPE(ar, ApplySquare);
}

REGISTER_TYPES(Task);
