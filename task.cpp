#include "stdafx.h"

#include "task.h"
#include "level.h"
#include "collective.h"
#include "entity_set.h"

template <class Archive> 
void Task::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(UniqueEntity)
    & SVAR(position)
    & SVAR(done)
    & SVAR(collective);
  CHECK_SERIAL;
}

SERIALIZABLE(Task);

Task::Task(Collective* col, Vec2 pos) : position(pos), collective(col) {}

Task::~Task() {
}

Vec2 Task::getPosition() {
  return position;
}

Collective* Task::getCollective() {
  return collective;
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

class Construction : public Task {
  public:
  Construction(Collective* col, Vec2 position, SquareType _type) : Task(col, position), type(_type) {}

  virtual bool isImpossible(const Level* level) {
    return !level->getSquare(getPosition())->canConstruct(type);
  }

  virtual string getInfo() override {
    return string("construct ") + typeid(type).name() + " " + convertToString(getPosition());
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!c->hasSkill(Skill::construction))
      return NoMove;
    Vec2 dir = getPosition() - c->getPosition();
    if (dir.length8() == 1) {
      if (c->canConstruct(dir, type))
        return {1.0, [this, c, dir] {
          c->construct(dir, type);
          if (!c->canConstruct(dir, type)) {
            setDone();
            getCollective()->onConstructed(getPosition(), type);
          }
        }};
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


PTask Task::construction(Collective* col, Vec2 target, SquareType type) {
  return PTask(new Construction(col, target, type));
}
  
MoveInfo Task::getMoveToPosition(Creature *c) {
  Optional<Vec2> move = c->getMoveTowards(position, true);
  if (move)
    return {1.0, [=] {
      c->move(*move);
    }};
  else
    return NoMove;
}

class PickItem : public Task {
  public:
  PickItem(Collective* col, Vec2 position, vector<Item*> _items) 
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
      getCollective()->onCantPickItem(items);
      if (hereItems.empty()) {
        setDone();
        return NoMove;
      }
      items = hereItems;
      if (c->canPickUp(hereItems))
        return {1.0, [=] {
          for (auto elem : Item::stackItems(hereItems))
            c->globalMessage(c->getTheName() + " picks up " + elem.first, "");
          c->pickUp(hereItems);
          pickedUp = true;
          onPickedUp();
          getCollective()->onPickedUp(getPosition(), hereItems);
        }}; 
      else {
        getCollective()->onCantPickItem(items);
        setDone();
        return NoMove;
      }
    }
    if (MoveInfo move = getMoveToPosition(c))
      return move;
    else if (c->getPosition().dist8(getPosition()) == 1) {
      getCollective()->onCantPickItem(items);
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
};

PTask Task::pickItem(Collective* col, Vec2 position, vector<Item*> items) {
  return PTask(new PickItem(col, position, items));
}

class EquipItem : public PickItem {
  public:
  EquipItem(Collective* col, Vec2 position, vector<Item*> _items) : PickItem(col, position, _items) {
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
    if (!it.empty() && c->canEquip(getOnlyElement(it)))
      return {1.0, [=] {
        c->globalMessage(c->getTheName() + " equips " + getOnlyElement(it)->getAName());
        c->equip(getOnlyElement(it));
        setDone();
      }};
    else {
      setDone();
      return NoMove;
    }
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(PickItem);
  }
  
  SERIALIZATION_CONSTRUCTOR(EquipItem);
};

PTask Task::equipItem(Collective* col, Vec2 position, Item* items) {
  return PTask(new EquipItem(col, position, {items}));
}

class BringItem : public PickItem {
  public:
  BringItem(Collective* col, Vec2 position, vector<Item*> items, Vec2 _target)
      : PickItem(col, position, items), target(_target) {}

  virtual void onBroughtItem(Creature* c, vector<Item*> it) {
    //getCollective()->onBrought(c->getPosition(), it);
    for (auto elem : Item::stackItems(it))
      c->globalMessage(c->getTheName() + " drops " + elem.first);
    c->drop(it);
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
      return {1.0, [=] {
        onBroughtItem(c, myItems);
        setDone();
      }};
    } else {
      if (c->getPosition().dist8(target) == 1)
        if (Creature* other = c->getLevel()->getSquare(target)->getCreature())
          if (other->isAffected(Creature::SLEEP))
            other->removeEffect(Creature::SLEEP);
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

PTask Task::bringItem(Collective* col, Vec2 position, vector<Item*> items, Vec2 target) {
  return PTask(new BringItem(col, position, items, target));
}

class ApplyItem : public BringItem {
  public:
  ApplyItem(Collective* col, Vec2 position, vector<Item*> items, Vec2 _target) 
      : BringItem(col, position, items, _target) {}

  virtual void cancel() override {
    getCollective()->onAppliedItemCancel(target);
  }

  virtual string getInfo() override {
    return "apply item from " + convertToString(getPosition()) +
      " to " + convertToString(target);
  }

  virtual void onBroughtItem(Creature* c, vector<Item*> it) override {
    Item* item = getOnlyElement(it);
    getCollective()->onAppliedItem(c->getPosition(), item);
    c->globalMessage(c->getTheName() + " " + item->getApplyMsgThirdPerson(),
        item->getNoSeeApplyMsg());
    c->applyItem(item);
  }

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version) {
    ar & SUBCLASS(BringItem);
  }
  
  SERIALIZATION_CONSTRUCTOR(ApplyItem);
};

PTask Task::applyItem(Collective* col, Vec2 position, Item* item, Vec2 target) {
  return PTask(new ApplyItem(col, position, {item}, target));
}

class ApplySquare : public Task {
  public:
  ApplySquare(Collective* col, set<Vec2> pos) : Task(col, Vec2(-1, 1)), positions(pos) {}

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
      if (c->getSquare()->getApplyType(c))
        return {1.0, [this, c] {
          c->applySquare();
          setDone();
          getCollective()->onAppliedSquare(c->getPosition());
        }};
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

PTask Task::applySquare(Collective* col, set<Vec2> position) {
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
