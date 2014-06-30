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
    & SVAR(position)
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
    & BOOST_SERIALIZATION_NVP(taken)
    & BOOST_SERIALIZATION_NVP(taskMap);
}

SERIALIZABLE(Task::Mapping);

Task::Task(Callback* call, Vec2 pos) : position(pos), callback(call) {}

Task::~Task() {
}

Vec2 Task::getPosition() {
  return position;
}

Task::Callback* Task::getCallback() {
  return callback;
}

bool Task::isDone() {
  return done;
}

void Task::setDone() {
  done = true;
}

void Task::setPosition(Vec2 pos) {
  position = pos;
}

void Task::Mapping::removeTask(Task* task) {
  for (int i : All(tasks))
    if (tasks[i].get() == task) {
      removeIndex(tasks, i);
      break;
    }
  if (taken.count(task)) {
    taskMap.erase(taken.at(task));
    taken.erase(task);
  }
}

void Task::Mapping::removeTask(UniqueId id) {
  for (PTask& task : tasks)
    if (task->getUniqueId() == id) {
      removeTask(task.get());
    }
}

Task* Task::Mapping::getTask(const Creature* c) const {
  if (taskMap.count(c))
    return taskMap.at(c);
  else
    return nullptr;
}

Task* Task::Mapping::addTask(PTask task, const Creature* c) {
  if (c) {
    taken[task.get()] = c;
    taskMap[c] = task.get();
  }
  tasks.push_back(std::move(task));
  return tasks.back().get();
}

void Task::Mapping::takeTask(const Creature* c, Task* task) {
  freeTask(task);
  taskMap[c] = task;
  taken[task] = c;
}

void Task::Mapping::freeTask(Task* task) {
  if (taken.count(task)) {
    taskMap.erase(taken.at(task));
    taken.erase(task);
  }
}

class Construction : public Task {
  public:
  Construction(Callback* col, Vec2 position, SquareType _type) : Task(col, position), type(_type) {}

  virtual bool isImpossible(const Level* level) {
    return !level->getSquare(getPosition())->canConstruct(type);
  }

  virtual string getInfo() override {
    return string("construct ") + typeid(type).name() + " " + convertToString(getPosition());
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!c->hasSkill(Skill::get(SkillId::CONSTRUCTION)))
      return NoMove;
    Vec2 dir = getPosition() - c->getPosition();
    if (dir.length8() == 1) {
      if (auto action = c->construct(dir, type))
        return {1.0, action.append([=] {
          if (!c->construct(dir, type)) {
            setDone();
            getCallback()->onConstructed(getPosition(), type);
          }
        })};
      else {
        setDone();
        return NoMove;
      }
    } else
        return getMoveToPosition(c);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Task)
      & SVAR(type);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Construction);

  private:
  SquareType SERIAL(type);
};


PTask Task::construction(Callback* col, Vec2 target, SquareType type) {
  return PTask(new Construction(col, target, type));
}
  
MoveInfo Task::getMoveToPosition(Creature *c, bool stepOnTile) {
  if (auto action = c->moveTowards(position, stepOnTile))
    return {1.0, action};
  else
    return NoMove;
}

class PickItem : public Task {
  public:
  PickItem(Callback* col, Vec2 position, vector<Item*> _items) 
      : Task(col, position), items(_items) {
  }

  virtual void onPickedUp() {
    setDone();
  }

  virtual bool canTransfer() override {
    return false;
  }

  virtual string getInfo() override {
    return "pick item " + convertToString(getPosition());
  }

  virtual MoveInfo getMove(Creature* c) override {
    CHECK(!pickedUp);
    if (c->getPosition() == getPosition()) {
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
          getCallback()->onPickedUp(getPosition(), hereItems);
        })}; 
      else {
        getCallback()->onCantPickItem(items);
        setDone();
        return NoMove;
      }
    }
    if (MoveInfo move = getMoveToPosition(c, true))
      return move;
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
      & SVAR(pickedUp);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(PickItem);

  protected:
  EntitySet SERIAL(items);
  bool SERIAL2(pickedUp, false);
  int tries = 10;
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

  virtual string getInfo() override {
    return "equip item " + convertToString(getPosition());
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

class BringItem : public PickItem {
  public:
  BringItem(Callback* col, Vec2 position, vector<Item*> items, Vec2 _target)
      : PickItem(col, position, items), target(_target) {}

  virtual CreatureAction getBroughtAction(Creature* c, vector<Item*> it) {
    return c->drop(it);
  }

  virtual string getInfo() override {
    return "bring item from " + convertToString(getPosition())
      + " to " + convertToString(target);
  }

  virtual void onPickedUp() override {
  }
  
  virtual MoveInfo getMove(Creature* c) override {
    if (!pickedUp)
      return PickItem::getMove(c);
    setPosition(target);
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
      return getMoveToPosition(c);
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

PTask Task::bringItem(Callback* col, Vec2 position, vector<Item*> items, Vec2 target) {
  return PTask(new BringItem(col, position, items, target));
}

class ApplyItem : public BringItem {
  public:
  ApplyItem(Callback* col, Vec2 position, vector<Item*> items, Vec2 _target) 
      : BringItem(col, position, items, _target) {}

  virtual void cancel() override {
    getCallback()->onAppliedItemCancel(target);
  }

  virtual string getInfo() override {
    return "apply item from " + convertToString(getPosition()) +
      " to " + convertToString(target);
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
  ApplySquare(Callback* col, set<Vec2> pos) : Task(col, Vec2(-1, 1)), positions(pos) {}

  virtual bool canTransfer() override {
    return false;
  }

  virtual string getInfo() override {
    return "apply square ";
  }

  virtual MoveInfo getMove(Creature* c) override {
    Vec2 pos = getPosition();
    if (pos.x == -1) {
      pos = Vec2(-10000, -10000);
      for (Vec2 v : randomPermutation(positions))
        if (!rejectedPosition.count(v) &&
            (pos - c->getPosition()).length8() > (v - c->getPosition()).length8() &&
            !c->getLevel()->getSquare(v)->getCreature())
          pos = v;
      if (pos.x == -10000) {
        setDone();
        return NoMove;
      }
      setPosition(pos);
    }
    if (c->getPosition() == getPosition()) {
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
      MoveInfo move = getMoveToPosition(c);
      if (!move || ((getPosition() - c->getPosition()).length8() == 1 && c->getLevel()->getSquare(getPosition())->getCreature())) {
        rejectedPosition.insert(getPosition());
        setPosition(Vec2(-1, -1));
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
      & SVAR(invalidCount);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(ApplySquare);

  private:
  set<Vec2> SERIAL(positions);
  set<Vec2> SERIAL(rejectedPosition);
  int SERIAL2(invalidCount, 5);
};

PTask Task::applySquare(Callback* col, set<Vec2> position) {
  CHECK(position.size() > 0);
  return PTask(new ApplySquare(col, position));
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
