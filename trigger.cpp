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

#include "trigger.h"
#include "level.h"
#include "effect.h"
#include "item.h"
#include "creature.h"
#include "square.h"
#include "view_id.h"
#include "view_object.h"
#include "item_factory.h"

template <class Archive> 
void Trigger::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(viewObject)
    & SVAR(level)
    & SVAR(position);
  CHECK_SERIAL;
}

SERIALIZABLE(Trigger);

SERIALIZATION_CONSTRUCTOR_IMPL(Trigger);

Trigger::Trigger(Level* l, Vec2 p) : level(l), position(p) {
}

Trigger::~Trigger() {}

Trigger::Trigger(const ViewObject& obj, Level* l, Vec2 p): viewObject(obj), level(l), position(p) {
}

Optional<ViewObject> Trigger::getViewObject(const CreatureView*) const {
  return viewObject;
}

void Trigger::onCreatureEnter(Creature* c) {}

void Trigger::setOnFire(double size) {}

double Trigger::getLightEmission() const {
  return 0;
}

bool Trigger::interceptsFlyingItem(Item* it) const { return false; }
void Trigger::onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir, Vision*) {}
bool Trigger::isDangerous(const Creature* c) const { return false; }
void Trigger::tick(double time) {}

namespace {

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
      c->playerMessage("The portal is not working.");
  }

  virtual bool interceptsFlyingItem(Item* it) const override {
    return other && !Random.roll(5);
  }

  virtual void onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir,
      Vision* vision) {
    string name = it[0]->getTheName(it.size() > 1);
    level->globalMessage(position, name + " disappears in the portal.");
    other->level->throwItem(std::move(it), a, remainingDist, other->position, dir, vision);
  }

  virtual void tick(double time) override {
    if (startTime == -1)
      startTime = time;
    if (time - startTime >= 30) {
      level->globalMessage(position, "The portal disappears.");
      level->getSquare(position)->removeTrigger(this);
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger)
      & SVAR(startTime)
      & SVAR(active)
      & SVAR(other)
      & SVAR(previous);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Portal);

  private:
  double SERIAL2(startTime, 1000000);
  bool SERIAL2(active, true);
  Portal* SERIAL2(other, nullptr);
  static Portal* SERIAL(previous);
};

}

Portal* Portal::previous = nullptr;
  
PTrigger Trigger::getPortal(const ViewObject& obj, Level* l, Vec2 position) {
  return PTrigger(new Portal(obj, l, position));
}

namespace {

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
      if (!c->hasSkill(Skill::get(SkillId::DISARM_TRAPS))) {
        c->you(MsgType::TRIGGER_TRAP, "");
        Effect::applyToCreature(c, effect, EffectStrength::NORMAL);
        GlobalEvents.addTrapTriggerEvent(c->getLevel(), c->getPosition());
      } else {
        c->you(MsgType::DISARM_TRAP, "");
        GlobalEvents.addTrapDisarmEvent(c->getLevel(), c, c->getPosition());
      }
      c->getSquare()->removeTrigger(this);
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger) 
      & SVAR(effect)
      & SVAR(tribe);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Trap);

  private:
  EffectType SERIAL(effect);
  Tribe* SERIAL(tribe);
};

}

PTrigger Trigger::getTrap(const ViewObject& obj, Level* l, Vec2 position, EffectType effect, Tribe* tribe) {
  return PTrigger(new Trap(obj, l, position, std::move(effect), tribe));
}

namespace {

class Torch : public Trigger {
  public:
  Torch(const ViewObject& obj, Level* l, Vec2 position) : Trigger(obj, l, position) {
  }

  virtual double getLightEmission() const override {
    return 8.2;
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(Torch);
};

}

const ViewObject& Trigger::getTorchViewObject(Dir dir) {
  static map<Dir, ViewObject> objs;
  if (objs.empty())
    for (Dir dir : ENUM_ALL(Dir))
      objs[dir] = ViewObject(ViewId::TORCH, dir == Dir::N ? ViewLayer::TORCH1 : ViewLayer::TORCH2, "Torch")
        .setAttachmentDir(dir);
  return objs[dir];
}

PTrigger Trigger::getTorch(Dir attachmentDir, Level* l, Vec2 position) {
  return PTrigger(new Torch(getTorchViewObject(attachmentDir), l, position));
}

namespace {

class MeteorShower : public Trigger {
  public:
  MeteorShower(Creature* c, double duration) : Trigger(c->getLevel(), c->getPosition()), creature(c),
      endTime(creature->getTime() + duration) {}

  virtual void tick(double time) override {
    if (time >= endTime || creature->isDead()) {
      level->getSquare(position)->removeTrigger(this);
      return;
    } else
      for (int i : Range(10))
        if (shootMeteorite())
          break;
  }

  const int areaWidth = 3;
  const int range = 4;

  bool shootMeteorite() {
    Vec2 targetPoint(Random.getRandom(-areaWidth / 2, areaWidth / 2 + 1),
                     Random.getRandom(-areaWidth / 2, areaWidth / 2 + 1));
    targetPoint += position;
    Vec2 direction(Random.getRandom(-1, 2), Random.getRandom(-1, 2));
    for (int i : Range(range + 1))
      if (!level->getSquare(targetPoint + direction * i)->canEnter(
            MovementType({MovementTrait::WALK, MovementTrait::FLY})))
        return false;
    level->throwItem(ItemFactory::fromId(ItemId::ROCK),
        Attack(creature, AttackLevel::MIDDLE, AttackType::HIT, 25, 40, false), 10,
        targetPoint + direction * range, -direction, Vision::get(VisionId::NORMAL));
    return true;
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger) & SVAR(creature) & SVAR(endTime);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(MeteorShower);

  private:
  Creature* SERIAL(creature);
  double SERIAL(endTime);
};

}

PTrigger Trigger::getMeteorShower(Creature* c, double duration) {
  return PTrigger(new MeteorShower(c, duration));
}

template <class Archive>
void Trigger::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, Torch);
  REGISTER_TYPE(ar, Trap);
  REGISTER_TYPE(ar, Portal);
}

REGISTER_TYPES(Trigger);
