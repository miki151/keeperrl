#include "stdafx.h"

#include "task.h"
#include "level.h"
#include "collective.h"

Task::Task(Collective* col, Vec2 pos) : position(pos), collective(col) {}

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

  private:
  SquareType type;
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
      : Task(col, position), items(_items.begin(), _items.end()) {
  }

  virtual void onPickedUp() {
    setDone();
  }

  virtual bool canTransfer() override {
    return false;
  }

  virtual string getInfo() override {
    return "pick item " + (*items.begin())->getName() + " " + convertToString(getPosition());
  }

  virtual MoveInfo getMove(Creature* c) override {
    CHECK(!pickedUp);
    if (c->getPosition() == getPosition()) {
      vector<Item*> hereItems;
      for (Item* it : c->getPickUpOptions())
        if (items.count(it)) {
          hereItems.push_back(it);
          items.erase(it);
        }
      getCollective()->onCantPickItem(vector<Item*>(items.begin(), items.end()));
      if (hereItems.empty()) {
        setDone();
        return NoMove;
      }
      items = set<Item*>(hereItems.begin(), hereItems.end());
      if (c->canPickUp(hereItems))
        return {1.0, [=] {
          c->pickUp(hereItems);
          pickedUp = true;
          onPickedUp();
          getCollective()->onPickedUp(getPosition(), hereItems);
        }}; 
      else {
        getCollective()->onCantPickItem(hereItems);
        return NoMove;
      }
    }
    else
      return getMoveToPosition(c);
  }

  protected:
  set<Item*> items;
  bool pickedUp = false;
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
    return "equip item " + (*items.begin())->getName() + " " + convertToString(getPosition());
  }

  virtual MoveInfo getMove(Creature* c) override {
    if (!pickedUp)
      return PickItem::getMove(c);
    CHECK(items.size() <= 1);
    for (Item* it : items)
      if (c->canEquip(it))
        return {1.0, [=] {
          c->equip(it);
          setDone();
        }};
    return NoMove;
  }
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
    c->drop(it);
  }

  // TODO: this fails when item doesn't exist anymore, use cautiously
  virtual string getInfo() override {
    return "bring item " + (*items.begin())->getName() + " " + convertToString(getPosition()) 
      + " to " + convertToString(target);
  }

  virtual void onPickedUp() override {
  }
  
  virtual MoveInfo getMove(Creature* c) override {
    if (!pickedUp)
      return PickItem::getMove(c);
    setPosition(target);
    if (c->getPosition() == target) {
      vector<Item*> myItems;
      for (Item* it : items)
        if (c->getEquipment().hasItem(it))
          myItems.push_back(it);
      return {1.0, [=] {
        onBroughtItem(c, myItems);
        setDone();
      }};
    } else {
      if (c->getPosition().dist8(target) == 1)
        if (Creature* other = c->getLevel()->getSquare(target)->getCreature())
          if (other->isSleeping())
            other->wakeUp();
      return getMoveToPosition(c);
    }
  }

  virtual bool canTransfer() override {
    return !pickedUp;
  }

  protected:
  Vec2 target;

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
    return "apply item " + (*items.begin())->getName() + " " + convertToString(getPosition()) +
      " to " + convertToString(target);
  }

  virtual void onBroughtItem(Creature* c, vector<Item*> it) override {
    CHECK(it.size() == 1);
    getCollective()->onAppliedItem(c->getPosition(), it[0]);
    c->applyItem(it[0]);
  }
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
      if (!move.isValid() || ((getPosition() - c->getPosition()).length8() == 1 && c->getLevel()->getSquare(getPosition())->getCreature())) {
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

  private:
  set<Vec2> positions;
  set<Vec2> rejectedPosition;
  int invalidCount = 5;
};

PTask Task::applySquare(Collective* col, set<Vec2> position) {
  CHECK(position.size() > 0);
  return PTask(new ApplySquare(col, position));
}

class Eat : public Task {
  public:
  Eat(Collective* col, set<Vec2> pos) : Task(col, Vec2(-1, 1)), positions(pos) {}

  virtual bool canTransfer() override {
    return false;
  }

  virtual string getInfo() override {
    return "eat";
  }

  Item* getDeadChicken(Square* s) {
    vector<Item*> chickens = s->getItems(Item::typePredicate(ItemType::FOOD));
    if (chickens.empty())
      return nullptr;
    else
      return chickens[0];
  }

  virtual MoveInfo getMove(Creature* c) override {
    Vec2 pos = getPosition();
    if (pos.x == -1) {
      pos = Vec2(-10000, -10000);
      for (Vec2 v : positions)
        if (!rejectedPosition.count(v) && (pos - c->getPosition()).length8() > (v - c->getPosition()).length8())
          pos = v;
      if (pos.x == -10000) {
        setDone();
        return NoMove;
      }
      setPosition(pos);
    }
    Item* chicken = getDeadChicken(c->getSquare());
    if (chicken) {
      return {1.0, [this, c, chicken] {
        c->globalMessage(c->getTheName() + " eats " + chicken->getAName());
        c->eat(chicken);
        setDone();
      }};
    }
    for (Vec2 v : Vec2::directions8(true)) {
      Item* chicken = getDeadChicken(c->getSquare(v));
      if (chicken && c->canMove(v))
        return {1.0, [this, c, v] {
          c->move(v);
        }};
      if (Creature* ch = c->getSquare(v)->getCreature())
        if (ch->getName() == "chicken" && c->canAttack(ch)) {
        return {1.0, [this, c, ch] {
          c->attack(ch);
        }};
      }
    }
    MoveInfo move = getMoveToPosition(c);
    if (!move.isValid() || ((getPosition() - c->getPosition()).length8() == 1 && c->getLevel()->getSquare(getPosition())->getCreature())) {
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

  private:
  set<Vec2> positions;
  set<Vec2> rejectedPosition;
  int invalidCount = 5;
};

PTask Task::eat(Collective* col, set<Vec2> hatcherySquares) {
  return PTask(new Eat(col, hatcherySquares));
}
