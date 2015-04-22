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
#include "model.h"

template <class Archive> 
void Trigger::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(viewObject)
    & SVAR(level)
    & SVAR(position);
}

SERIALIZABLE(Trigger);

SERIALIZATION_CONSTRUCTOR_IMPL(Trigger);

Trigger::Trigger(Level* l, Vec2 p) : level(l), position(p) {
}

Trigger::~Trigger() {}

Trigger::Trigger(const ViewObject& obj, Level* l, Vec2 p): viewObject(obj), level(l), position(p) {
}

optional<ViewObject> Trigger::getViewObject(const Tribe*) const {
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

  Portal* getOther(Model::PortalInfo info) const {
    for (Trigger* t : info.level()->getSafeSquare(info.position())->getTriggers())
      if (Portal* ret = dynamic_cast<Portal*>(t))
        return ret;
    return nullptr;
  }

  Portal* getOther() const {
    if (otherPortal)
      return getOther(*otherPortal);
    return nullptr;
  }

  Portal(const ViewObject& obj, Level* l, Vec2 position) : Trigger(obj, l, position) {
    if (auto portalInfo = l->getModel()->getDanglingPortal())
      if (Portal* previous = getOther(*portalInfo)) {
        otherPortal = portalInfo;
        previous->otherPortal = Model::PortalInfo(l, position);
        l->getModel()->resetDanglingPortal();
        startTime = previous->startTime = -1;
        return;
      }
    l->getModel()->setDanglingPortal(Model::PortalInfo(l, position));
  }

  virtual void onCreatureEnter(Creature* c) override {
    if (!active) {
      active = true;
      return;
    }
    if (Portal* other = getOther()) {
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
      c->playerMessage("The portal is inactive. Create another one to open a connection.");
  }

  virtual bool interceptsFlyingItem(Item* it) const override {
    return getOther() && !Random.roll(5);
  }

  virtual void onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir,
      VisionId vision) {
    string name = it[0]->getTheName(it.size() > 1);
    level->globalMessage(position, name + " disappears in the portal.");
    NOTNULL(getOther())->level->throwItem(std::move(it), a, remainingDist, NOTNULL(getOther())->position, dir, vision);
  }

  virtual void tick(double time) override {
    if (startTime == -1)
      startTime = time;
    if (time - startTime >= 30) {
      level->globalMessage(position, "The portal disappears.");
      level->getSafeSquare(position)->removeTrigger(this);
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger)
      & SVAR(startTime)
      & SVAR(active);
    if (version >= 1)
      ar & SVAR(otherPortal);
    else {
      Portal* p; // SERIAL(p)
      ar & SVAR(p);
      if (p)
        otherPortal = Model::PortalInfo(p->level, p->position);
    }
  }

  SERIALIZATION_CONSTRUCTOR(Portal);

  private:
  double SERIAL(startTime) = 1000000;
  bool SERIAL(active) = true;
  optional<Model::PortalInfo> SERIAL(otherPortal);
};

}

BOOST_CLASS_VERSION(Portal, 1)

PTrigger Trigger::getPortal(const ViewObject& obj, Level* l, Vec2 position) {
  return PTrigger(new Portal(obj, l, position));
}

namespace {

class Trap : public Trigger {
  public:
  Trap(const ViewObject& obj, Level* l, Vec2 position, EffectType _effect, Tribe* _tribe)
      : Trigger(obj, l, position), effect(_effect), tribe(_tribe) {
  }

  virtual optional<ViewObject> getViewObject(const Tribe* t) const {
    if (t == tribe)
      return viewObject;
    else
      return none;
  }

  virtual bool isDangerous(const Creature* c) const override {
    return c->getTribe() != tribe;
  }

  virtual void onCreatureEnter(Creature* c) override {
    if (c->getTribe() != tribe) {
      if (!c->hasSkill(Skill::get(SkillId::DISARM_TRAPS))) {
        c->you(MsgType::TRIGGER_TRAP, "");
        Effect::applyToCreature(c, effect, EffectStrength::NORMAL);
        level->getModel()->onTrapTrigger(c->getLevel(), c->getPosition());
      } else {
        c->you(MsgType::DISARM_TRAP, "");
        level->getModel()->onTrapDisarm(c->getLevel(), c, c->getPosition());
      }
      c->getSquare()->removeTrigger(this);
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger) 
      & SVAR(effect)
      & SVAR(tribe);
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
      level->getSafeSquare(position)->removeTrigger(this);
      return;
    } else
      for (int i : Range(10))
        if (shootMeteorite())
          break;
  }

  const int areaWidth = 3;
  const int range = 4;

  bool shootMeteorite() {
    Vec2 targetPoint(Random.get(-areaWidth / 2, areaWidth / 2 + 1),
                     Random.get(-areaWidth / 2, areaWidth / 2 + 1));
    targetPoint += position;
    Vec2 direction(Random.get(-1, 2), Random.get(-1, 2));
    if (!level->inBounds(targetPoint) || direction.length8() == 0)
      return false;
    for (int i : Range(range + 1))
      if (!level->getSafeSquare(targetPoint + direction * i)->canEnter(
            MovementType({MovementTrait::WALK, MovementTrait::FLY})))
        return false;
    level->throwItem(ItemFactory::fromId(ItemId::ROCK),
        Attack(creature, AttackLevel::MIDDLE, AttackType::HIT, 25, 40, false), 10,
        targetPoint + direction * range, -direction, VisionId::NORMAL);
    return true;
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Trigger) & SVAR(creature) & SVAR(endTime);
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
void Trigger::registerTypes(Archive& ar, int version) {
  REGISTER_TYPE(ar, Torch);
  REGISTER_TYPE(ar, Trap);
  REGISTER_TYPE(ar, Portal);
  REGISTER_TYPE(ar, MeteorShower);
}

REGISTER_TYPES(Trigger::registerTypes);
