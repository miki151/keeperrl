#include "stdafx.h"

#include "trigger.h"
#include "level.h"

template <class Archive> 
void Trigger::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(viewObject)
    & BOOST_SERIALIZATION_NVP(level)
    & BOOST_SERIALIZATION_NVP(position);
}

SERIALIZABLE(Trigger);

Trigger::Trigger(Level* l, Vec2 p) : level(l), position(p) {
}

Trigger::~Trigger() {}

Trigger::Trigger(const ViewObject& obj, Level* l, Vec2 p): viewObject(obj), level(l), position(p) {
}

Optional<ViewObject> Trigger::getViewObject(const CreatureView*) const {
  return viewObject;
}

void Trigger::onCreatureEnter(Creature* c) {}

bool Trigger::interceptsFlyingItem(Item* it) const { return false; }
void Trigger::onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir) {}
bool Trigger::isDangerous(const Creature* c) const { return false; }
void Trigger::tick(double time) {}

class Portal : public Trigger {
  public:
  Portal(const ViewObject& obj, Level* l, Vec2 position) : Trigger(obj, l, position) {
    if (previous) {
      other = previous;
      other->other = this;
      previous = nullptr;
      startTime = other->startTime = -1;
    } else
      previous = this;
  }

  virtual void onCreatureEnter(Creature* c) override {
    if (!active) {
      active = true;
      return;
    }
    if (other) {
      other->active = false;
      c->you(MsgType::ENTER_PORTAL, "");
      if (other->level == level) {
        if (level->canMoveCreature(c, other->position - c->getPosition())) {
          level->moveCreature(c, other->position - c->getPosition());
          return;
        }
        for (Vec2 v : other->position.neighbors8())
          if (level->canMoveCreature(c, v - c->getPosition())) {
            level->moveCreature(c, v - c->getPosition());
            return;
          }
      } else
        level->changeLevel(other->level, other->position, c);
    } else 
      c->privateMessage("The portal is not working.");
  }

  virtual bool interceptsFlyingItem(Item* it) const override {
    return other && !Random.roll(5);
  }

  virtual void onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir) {
    string name = it[0]->getTheName(it.size() > 1);
    level->globalMessage(position, name + " disappears in the portal.");
    other->level->throwItem(std::move(it), a, remainingDist, other->position, dir);
  }

  virtual void tick(double time) override {
    if (startTime == -1)
      startTime = time;
    if (time - startTime >= 100) {
      level->globalMessage(position, "The portal disappears.");
      level->getSquare(position)->removeTrigger(this);
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger)
      & BOOST_SERIALIZATION_NVP(startTime)
      & BOOST_SERIALIZATION_NVP(active)
      & BOOST_SERIALIZATION_NVP(other)
      & BOOST_SERIALIZATION_NVP(previous);
  }

  SERIALIZATION_CONSTRUCTOR(Portal);

  private:
  double startTime = 1000000;
  bool active = true;
  Portal* other = nullptr;
  static Portal* previous;
};

Portal* Portal::previous = nullptr;
  
PTrigger Trigger::getPortal(const ViewObject& obj, Level* l, Vec2 position) {
  return PTrigger(new Portal(obj, l, position));
}

class Trap : public Trigger {
  public:
  Trap(const ViewObject& obj, Level* l, Vec2 position, EffectType _effect, Tribe* _tribe)
      : Trigger(obj, l, position), effect(_effect), tribe(_tribe) {
  }

  virtual Optional<ViewObject> getViewObject(const CreatureView* c) const {
    if (c->getTribe() == tribe)
      return viewObject;
    else
      return Nothing();
  }

  virtual bool isDangerous(const Creature* c) const override {
    return c->getTribe() != tribe;
  }

  virtual void onCreatureEnter(Creature* c) override {
    if (c->getTribe() != tribe) {
      c->you(MsgType::TRIGGER_TRAP, "");
      Effect::applyToCreature(c, effect, EffectStrength::NORMAL);
      EventListener::addTriggerEvent(c->getLevel(), c->getPosition());
      c->getSquare()->removeTrigger(this);
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger) 
      & BOOST_SERIALIZATION_NVP(effect)
      & BOOST_SERIALIZATION_NVP(tribe);
  }

  SERIALIZATION_CONSTRUCTOR(Trap);

  private:
  EffectType effect;
  Tribe* tribe;
};

template <class Archive>
void Trigger::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, Trap);
  REGISTER_TYPE(ar, Portal);
}

REGISTER_TYPES(Trigger);

PTrigger Trigger::getTrap(const ViewObject& obj, Level* l, Vec2 position, EffectType effect, Tribe* tribe) {
  return PTrigger(new Trap(obj, l, position, std::move(effect), tribe));
}
