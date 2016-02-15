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
#include "view_id.h"
#include "view_object.h"
#include "item_factory.h"
#include "model.h"
#include "attack.h"
#include "player_message.h"
#include "tribe.h"
#include "skill.h"
#include "modifier_type.h"

template <class Archive> 
void Trigger::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(viewObject)
    & SVAR(position);
}

SERIALIZABLE(Trigger);

SERIALIZATION_CONSTRUCTOR_IMPL(Trigger);

Trigger::Trigger(Position p) : position(p) {
}

Trigger::~Trigger() {}

Trigger::Trigger(const ViewObject& obj, Position p): viewObject(obj), position(p) {
}

optional<ViewObject> Trigger::getViewObject(TribeId) const {
  return viewObject;
}

void Trigger::onCreatureEnter(Creature* c) {}

void Trigger::setOnFire(double size) {}

double Trigger::getLightEmission() const {
  return 0;
}

bool Trigger::interceptsFlyingItem(Item* it) const { return false; }
void Trigger::onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir, VisionId) {}
bool Trigger::isDangerous(const Creature* c) const { return false; }
void Trigger::tick(double time) {}

namespace {

class Portal : public Trigger {
  public:

  Portal* getOther(Position info) const {
    for (Trigger* t : info.getTriggers())
      if (Portal* ret = dynamic_cast<Portal*>(t))
        return ret;
    return nullptr;
  }

  Portal* getOther() const {
    if (otherPortal)
      return getOther(*otherPortal);
    return nullptr;
  }

  Portal(const ViewObject& obj, Position position) : Trigger(obj, position) {
    if (auto danglingPortal = position.getModel()->getDanglingPortal())
      if (Portal* previous = getOther(*danglingPortal)) {
        otherPortal = danglingPortal;
        previous->otherPortal = position;
        position.getModel()->resetDanglingPortal();
        startTime = previous->startTime = -1;
        return;
      }
    position.getModel()->setDanglingPortal(position);
  }

  virtual void onCreatureEnter(Creature* c) override {
    if (!active) {
      active = true;
      return;
    }
    if (Portal* other = getOther()) {
      other->active = false;
      c->you(MsgType::ENTER_PORTAL, "");
      if (position.canMoveCreature(other->position)) {
        position.moveCreature(other->position);
        return;
      }
      for (Position v : other->position.neighbors8())
        if (position.canMoveCreature(v)) {
          position.moveCreature(v);
          return;
        }
    } else
      c->playerMessage("The portal is inactive. Create another one to open a connection.");
  }

  virtual bool interceptsFlyingItem(Item* it) const override {
    return getOther() && !Random.roll(5);
  }

  virtual void onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir,
      VisionId vision) override {
    position.globalMessage(it[0]->getPluralTheNameAndVerb(it.size(), "disappears", "disappear") +
        " in the portal.");
    NOTNULL(getOther())->position.throwItem(std::move(it), a, remainingDist, dir, vision);
  }

  virtual void tick(double time) override {
    if (startTime == -1)
      startTime = time;
    if (time - startTime >= 30) {
      position.globalMessage("The portal disappears.");
      position.removeTrigger(this);
    }
  }

  SERIALIZE_ALL2(Trigger, startTime, active, otherPortal);

  SERIALIZATION_CONSTRUCTOR(Portal);

  private:
  double SERIAL(startTime) = 1000000;
  bool SERIAL(active) = true;
  optional<Position> SERIAL(otherPortal);
};

}

PTrigger Trigger::getPortal(const ViewObject& obj, Position position) {
  return PTrigger(new Portal(obj, position));
}

namespace {

class Trap : public Trigger {
  public:
  Trap(const ViewObject& obj, Position position, EffectType _effect, TribeId _tribe, bool visible)
      : Trigger(obj, position), effect(_effect), tribe(_tribe), alwaysVisible(visible) {
  }

  virtual optional<ViewObject> getViewObject(TribeId t) const override {
    if (alwaysVisible || t == tribe)
      return viewObject;
    else
      return none;
  }

  virtual bool isDangerous(const Creature* c) const override {
    return c->getTribeId() != tribe;
  }

  virtual void onCreatureEnter(Creature* c) override {
    if (c->getTribeId() != tribe) {
      if (!c->hasSkill(Skill::get(SkillId::DISARM_TRAPS))) {
        if (!alwaysVisible)
          c->you(MsgType::TRIGGER_TRAP, "");
        Effect::applyToCreature(c, effect, EffectStrength::NORMAL);
        position.getModel()->onTrapTrigger(c->getPosition());
      } else {
        c->you(MsgType::DISARM_TRAP, "");
        position.getModel()->onTrapDisarm(c->getPosition(), c);
      }
      c->getPosition().removeTrigger(this);
    }
  }

  SERIALIZE_ALL2(Trigger, effect, tribe, alwaysVisible);
  SERIALIZATION_CONSTRUCTOR(Trap);

  private:
  EffectType SERIAL(effect);
  TribeId SERIAL(tribe);
  bool SERIAL(alwaysVisible);
};

}

PTrigger Trigger::getTrap(const ViewObject& obj, Position pos, EffectType e, TribeId tribe, bool alwaysVisible) {
  return PTrigger(new Trap(obj, pos, std::move(e), tribe, alwaysVisible));
}

namespace {

class Torch : public Trigger {
  public:
  Torch(const ViewObject& obj, Position position) : Trigger(obj, position) {
  }

  virtual double getLightEmission() const override {
    return 8.2;
  }

  SERIALIZE_SUBCLASS(Trigger);
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

PTrigger Trigger::getTorch(Dir attachmentDir, Position position) {
  return PTrigger(new Torch(getTorchViewObject(attachmentDir), position));
}

namespace {

class MeteorShower : public Trigger {
  public:
  MeteorShower(Creature* c, double duration) : Trigger(c->getPosition()), creature(c),
      endTime(creature->getTime() + duration) {}

  virtual void tick(double time) override {
    if (time >= endTime || creature->isDead()) {
      position.removeTrigger(this);
      return;
    } else
      for (int i : Range(10))
        if (shootMeteorite())
          break;
  }

  const int areaWidth = 3;
  const int range = 4;

  bool shootMeteorite() {
    Position targetPoint = position.plus(Vec2(Random.get(-areaWidth / 2, areaWidth / 2 + 1),
                     Random.get(-areaWidth / 2, areaWidth / 2 + 1)));
    Vec2 direction(Random.get(-1, 2), Random.get(-1, 2));
    if (!targetPoint.isValid() || direction.length8() == 0)
      return false;
    for (int i : Range(range + 1))
      if (!targetPoint.plus(direction * i).canEnter(MovementType({MovementTrait::WALK, MovementTrait::FLY})))
        return false;
    targetPoint.plus(direction * range).throwItem(ItemFactory::fromId(ItemId::ROCK),
        Attack(creature, AttackLevel::MIDDLE, AttackType::HIT, 25, 40, false), 10, -direction, VisionId::NORMAL);
    return true;
  }

  SERIALIZE_ALL2(Trigger, creature, endTime);
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
void Trigger::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, Torch);
  REGISTER_TYPE(ar, Trap);
  REGISTER_TYPE(ar, Portal);
  REGISTER_TYPE(ar, MeteorShower);
}

REGISTER_TYPES(Trigger::registerTypes);
