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
#include "collective.h"

template <class Archive> 
void Task::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(UniqueEntity)
    & SVAR(done);
  CHECK_SERIAL;
}

SERIALIZABLE(Task);

template <class Archive> 
void Task::Callback::serialize(Archive& ar, const unsigned int version) {
}

SERIALIZABLE(Task::Callback);
SERIALIZATION_CONSTRUCTOR_IMPL2(Task, Callback);

Task::Task() {
}

Task::~Task() {
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

class Construction : public Task {
  public:
  Construction(Callback* c, Vec2 pos, SquareType _type) : type(_type), position(pos), callback(c) {}

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
            callback->onConstructed(position, type);
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
      & SVAR(type)
      & SVAR(callback);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Construction);

  private:
  SquareType SERIAL(type);
  Vec2 SERIAL(position);
  Callback* SERIAL(callback);
};

namespace {

class NonTransferable : public Task {
  public:

  virtual bool canTransfer() override {
    return false;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(Task);
  }
};

}

PTask Task::construction(Callback* c, Vec2 target, SquareType type) {
  return PTask(new Construction(c, target, type));
}

class PickItem : public NonTransferable {
  public:
  PickItem(Callback* c, Vec2 pos, vector<Item*> _items) : items(_items), position(pos), callback(c) {}

  virtual void onPickedUp() {
    setDone();
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
      callback->onCantPickItem(items);
      if (hereItems.empty()) {
        setDone();
        return NoMove;
      }
      items = hereItems;
      if (auto action = c->pickUp(hereItems))
        return {1.0, action.append([=] {
          pickedUp = true;
          onPickedUp();
          callback->onPickedUp(position, hereItems);
        })}; 
      else {
        callback->onCantPickItem(items);
        setDone();
        return NoMove;
      }
    }
    if (auto action = c->moveTowards(position, true))
      return action;
    else if (--tries == 0) {
      callback->onCantPickItem(items);
      setDone();
    }
    return NoMove;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(items)
      & SVAR(pickedUp)
      & SVAR(position)
      & SVAR(tries)
      & SVAR(callback);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(PickItem);

  protected:
  EntitySet<Item> SERIAL(items);
  bool SERIAL2(pickedUp, false);
  Vec2 SERIAL(position);
  int SERIAL2(tries, 10);
  Callback* SERIAL(callback);
};

PTask Task::pickItem(Callback* c, Vec2 position, vector<Item*> items) {
  return PTask(new PickItem(c, position, items));
}

class EquipItem : public PickItem {
  public:
  EquipItem(Callback* c, Vec2 position, vector<Item*> _items) : PickItem(c, position, _items) {
  }

  virtual void onPickedUp() override {
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!pickedUp)
      return PickItem::getMove(c);
    vector<Item*> it = c->getEquipment().getItems(items.containsPredicate());
    if (!it.empty()) {
      if (auto action = c->equip(getOnlyElement(it)))
        return {1.0, action.append([=] {
          setDone();
        })};
    } else
      setDone();
    return NoMove;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(PickItem);
  }
  
  SERIALIZATION_CONSTRUCTOR(EquipItem);
};

PTask Task::equipItem(Callback* c, Vec2 position, Item* items) {
  return PTask(new EquipItem(c, position, {items}));
}

static Vec2 chooseRandomClose(Vec2 start, const vector<Vec2>& squares) {
  int minD = 10000;
  int margin = 3;
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
  BringItem(Callback* c, Vec2 position, vector<Item*> items, vector<Vec2> _target)
      : PickItem(c, position, items), target(chooseRandomClose(position, _target)) {}

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

PTask Task::bringItem(Callback* c, Vec2 position, vector<Item*> items, vector<Vec2> target) {
  return PTask(new BringItem(c, position, items, target));
}

class ApplyItem : public BringItem {
  public:
  ApplyItem(Callback* c, Vec2 position, vector<Item*> items, Vec2 _target) 
      : BringItem(c, position, items, {_target}), callback(c) {}

  virtual void cancel() override {
    callback->onAppliedItemCancel(target);
  }

  virtual CreatureAction getBroughtAction(Creature* c, vector<Item*> it) override {
    Item* item = getOnlyElement(it);
    return c->applyItem(item).prepend([=] {
        callback->onAppliedItem(c->getPosition(), item);
    });
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(BringItem)
      & SVAR(callback);
  }
  
  SERIALIZATION_CONSTRUCTOR(ApplyItem);

  private:
  Callback* SERIAL(callback);
};

PTask Task::applyItem(Callback* c, Vec2 position, Item* item, Vec2 target) {
  return PTask(new ApplyItem(c, position, {item}, target));
}

class ApplySquare : public NonTransferable {
  public:
  ApplySquare(Callback* c, vector<Vec2> pos) : positions(pos), callback(c) {}

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
            if (callback)
              callback->onAppliedSquare(c->getPosition());
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
    ar& SUBCLASS(NonTransferable)
      & SVAR(positions)
      & SVAR(rejectedPosition)
      & SVAR(invalidCount)
      & SVAR(position)
      & SVAR(callback);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(ApplySquare);

  private:
  vector<Vec2> SERIAL(positions);
  set<Vec2> SERIAL(rejectedPosition);
  int SERIAL2(invalidCount, 5);
  Vec2 SERIAL2(position, Vec2(-1, -1));
  Callback* SERIAL(callback);
};

PTask Task::applySquare(Callback* c, vector<Vec2> position) {
  CHECK(position.size() > 0);
  return PTask(new ApplySquare(c, position));
}

namespace {

class Kill : public NonTransferable {
  public:
  enum Type { ATTACK, TORTURE };
  Kill(Callback* call, Creature* c, Type t) : creature(c), type(t), callback(call) {}

  CreatureAction getAction(Creature* c) {
    switch (type) {
      case ATTACK: return c->attack(creature);
      case TORTURE: return c->torture(creature);
    }
    return CreatureAction();
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (creature->isDead()) {
      setDone();
      return NoMove;
    }
    if (auto action = getAction(c))
      return action.append([=] { if (creature->isDead()) setDone(); });
    else
      return c->moveTowards(creature->getPosition());
  }

  virtual void cancel() override {
    callback->onKillCancelled(creature);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(creature)
      & SVAR(type)
      & SVAR(callback);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Kill);

  private:
  Creature* SERIAL(creature);
  Type SERIAL(type);
  Callback* SERIAL(callback);
};

}

PTask Task::kill(Callback* callback, Creature* creature) {
  return PTask(new Kill(callback, creature, Kill::ATTACK));
}

PTask Task::torture(Callback* callback, Creature* creature) {
  return PTask(new Kill(callback, creature, Kill::TORTURE));
}

namespace {

class Sacrifice : public NonTransferable {
  public:
  Sacrifice(Callback* call, Creature* c) : creature(c), callback(call) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (creature->isDead()) {
      if (sacrificePos) {
        if (sacrificePos == c->getPosition())
          return c->applySquare().append([=] { setDone(); });
        else
          return c->moveTowards(*sacrificePos);
      } else {
        setDone();
        return NoMove;
      }
    }
    if (creature->getSquare()->getApplyType(creature) == SquareApplyType::PRAY)
      if (auto action = c->attack(creature)) {
        Vec2 pos = creature->getPosition();
        return action.append([=] { if (creature->isDead()) sacrificePos = pos; });
      }
    return c->moveTowards(creature->getPosition());
  }

  virtual void cancel() override {
    callback->onKillCancelled(creature);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(creature)
      & SVAR(sacrificePos)
      & SVAR(callback);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Sacrifice);

  private:
  Creature* SERIAL(creature);
  Optional<Vec2> SERIAL(sacrificePos);
  Callback* SERIAL(callback);
};

}

PTask Task::sacrifice(Callback* callback, Creature* creature) {
  return PTask(new Sacrifice(callback, creature));
}

namespace {

class DestroySquare : public NonTransferable {
  public:
  DestroySquare(Vec2 pos) : position(pos) {
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (c->getPosition().dist8(position) == 1)
      if (auto action = c->destroy(position - c->getPosition(), Creature::DESTROY))
        return action.append([=] { 
          if (!c->getLevel()->getSquare(position)->canDestroyBy(c))
            setDone();
          });
    if (auto action = c->moveTowards(position))
      return action;
    for (Vec2 v : Vec2::directions8(true))
      if (auto action = c->destroy(v, Creature::DESTROY))
        return action;
    return NoMove;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(position);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(DestroySquare);

  private:
  Vec2 SERIAL(position);
};

}

PTask Task::destroySquare(Vec2 position) {
  return PTask(new DestroySquare(position));
}

namespace {

class Disappear : public NonTransferable {
  public:
  using NonTransferable::NonTransferable;

  virtual MoveInfo getMove(Creature* c) override {
    return c->disappear();
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable);
  }
};

}

PTask Task::disappear() {
  return PTask(new Disappear());
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

  virtual MoveInfo getMove(Creature* c) override {
    MoveInfo ret = tasks[current]->getMove(c);
    if (ret)
      ret.move.append([=] { 
          if (tasks[current]->isDone())
            ++current;
          if (current >= tasks.size())
            setDone();
          });
    return ret;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Task)
      & SVAR(tasks)
      & SVAR(current);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Chain);

  private:
  vector<PTask> SERIAL(tasks);
  int SERIAL2(current, 0);
};

}

PTask Task::chain(PTask t1, PTask t2) {
  return PTask(new Chain(makeVec<PTask>(std::move(t1), std::move(t2))));
}

PTask Task::chain(vector<PTask> v) {
  return PTask(new Chain(std::move(v)));
}

namespace {

class Explore : public NonTransferable {
  public:
  Explore(Vec2 pos) : position(pos) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (!Random.roll(3))
      return NoMove;
    if (auto action = c->moveTowards(position))
      return action.append([=] {
          if (c->getPosition().dist8(position) < 5)
            setDone();
      });
    if (Random.roll(3))
      setDone();
    return NoMove;
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(position);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Explore);

  private:
  Vec2 SERIAL(position);
};

}

PTask Task::explore(Vec2 pos) {
  return PTask(new Explore(pos));
}

namespace {

class AttackCollective : public NonTransferable {
  public:
  AttackCollective(Collective* col) : collective(col) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (c->getLevel() != collective->getLevel())
      return NoMove;
    if (auto action = c->moveTowards(collective->getLeader()->getPosition())) 
      return action;
    else {
      for (Vec2 v : Vec2::directions8(true))
        if (auto action = c->destroy(v, Creature::BASH))
          return action;
      return c->wait();
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(collective);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(AttackCollective);

  private:
  Collective* SERIAL(collective);
};

}

PTask Task::attackCollective(Collective* col) {
  return PTask(new AttackCollective(col));
}


namespace {

class CreateBed : public NonTransferable {
  public:
  CreateBed(Callback* call, Vec2 pos, SquareType from, SquareType to)
    : callback(call), position(pos), fromType(from), toType(to) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (c->getPosition() != position)
      return c->moveTowards(position);
    else
      return c->wait().append([=] {
        if (c->getPosition() == position) {
          c->getLevel()->replaceSquare(position, SquareFactory::get(toType));
          callback->onBedCreated(position, fromType, toType);
          setDone();
        }
      });
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(position)
      & SVAR(fromType)
      & SVAR(toType)
      & SVAR(callback);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(CreateBed);

  private:
  Callback* SERIAL(callback);
  Vec2 SERIAL(position);
  SquareType SERIAL(fromType);
  SquareType SERIAL(toType);
};

}

PTask Task::createBed(Callback* call, Vec2 pos, SquareType from, SquareType to) {
  return PTask(new CreateBed(call, pos, from, to));
}

namespace {
class ConsumeItem : public NonTransferable {
  public:
  ConsumeItem(Callback* c, vector<Item*> _items) : items(_items), callback(c) {}

  virtual MoveInfo getMove(Creature* c) override {
    return c->wait().append([=] {
        c->getEquipment().removeItems(c->getEquipment().getItems(items.containsPredicate())); });
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(items)
      & SVAR(callback);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(ConsumeItem);

  protected:
  EntitySet<Item> SERIAL(items);
  Callback* SERIAL(callback);
};
}

PTask Task::consumeItem(Callback* c, vector<Item*> items) {
  return PTask(new ConsumeItem(c, items));
}

namespace {
class Copulate : public NonTransferable {
  public:
  Copulate(Callback* c, Creature* t, int turns) : target(t), callback(c), numTurns(turns) {}

  virtual MoveInfo getMove(Creature* c) override {
    if (target->isDead() || !target->isAffected(LastingEffect::SLEEP)) {
      setDone();
      return NoMove;
    }
    if (c->getPosition().dist8(target->getPosition()) == 1) {
      if (Random.roll(2))
        for (Vec2 v : Vec2::directions8(true))
          if (v.dist8(target->getPosition() - c->getPosition()) == 1)
            if (auto action = c->move(v))
              return action;
      if (auto action = c->copulate(target->getPosition() - c->getPosition()))
        return action.append([=] {
          if (--numTurns == 0) {
            setDone();
            callback->onCopulated(c, target);
          }});
      else
        return NoMove;
    } else
      return c->moveTowards(target->getPosition());
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(NonTransferable)
      & SVAR(target)
      & SVAR(numTurns)
      & SVAR(callback);
    CHECK_SERIAL;
  }
  
  SERIALIZATION_CONSTRUCTOR(Copulate);

  protected:
  Creature* SERIAL(target);
  Callback* SERIAL(callback);
  int SERIAL(numTurns);
};
}

PTask Task::copulate(Callback* c, Creature* target, int numTurns) {
  return PTask(new Copulate(c, target, numTurns));
}

template <class Archive>
void Task::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, Construction);
  REGISTER_TYPE(ar, PickItem);
  REGISTER_TYPE(ar, EquipItem);
  REGISTER_TYPE(ar, BringItem);
  REGISTER_TYPE(ar, ApplyItem);
  REGISTER_TYPE(ar, ApplySquare);
  REGISTER_TYPE(ar, Kill);
  REGISTER_TYPE(ar, Sacrifice);
  REGISTER_TYPE(ar, DestroySquare);
  REGISTER_TYPE(ar, Disappear);
  REGISTER_TYPE(ar, Chain);
  REGISTER_TYPE(ar, Explore);
  REGISTER_TYPE(ar, AttackCollective);
  REGISTER_TYPE(ar, CreateBed);
  REGISTER_TYPE(ar, ConsumeItem);
  REGISTER_TYPE(ar, Copulate);
}

REGISTER_TYPES(Task);
