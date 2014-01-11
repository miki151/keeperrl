#include "stdafx.h"

Trigger::Trigger(Level* l, Vec2 p) : level(l), position(p) {
}

Trigger::Trigger(const ViewObject& obj, Level* l, Vec2 p): viewObject(obj), level(l), position(p) {
}

Optional<ViewObject> Trigger::getViewObject() const {
  return viewObject;
}

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

  virtual void onInterceptFlyingItem(PItem it, const Attack& a, int remainingDist, Vec2 dir) {
    level->globalMessage(position, it->getTheName() + " disappears in the portal.");
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
  Trap(const ViewObject& obj, Level* l, Vec2 position, PEffect _effect, Tribe* _tribe)
      : Trigger(obj, l, position), effect(std::move(_effect)), tribe(_tribe) {
  }

  virtual void onCreatureEnter(Creature* c) override {
    if (c->getTribe() != tribe) {
      c->you(MsgType::TRIGGER_TRAP, "");
      effect->applyToCreature(c, EffectStrength::NORMAL);
      EventListener::addTriggerEvent(c->getLevel(), c->getPosition());
      c->getSquare()->removeTrigger(this);
    }
  }

  private:
  PEffect effect;
  Tribe* tribe;
};
  
PTrigger Trigger::getTrap(const ViewObject& obj, Level* l, Vec2 position, PEffect effect, Tribe* tribe) {
  return PTrigger(new Trap(obj, l, position, std::move(effect), tribe));
}
