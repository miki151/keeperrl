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
#include "game.h"
#include "attack.h"
#include "player_message.h"
#include "tribe.h"
#include "skill.h"
#include "modifier_type.h"
#include "creature_attributes.h"
#include "attack_level.h"
#include "attack_type.h"
#include "event_listener.h"
#include "item_type.h"

SERIALIZE_DEF(Trigger, SUBCLASS(OwnedObject<Trigger>), viewObject, position)
SERIALIZATION_CONSTRUCTOR_IMPL(Trigger);

Trigger::Trigger(Position p) : position(p) {
}

Trigger::~Trigger() {}

Trigger::Trigger(const ViewObject& obj, Position p): viewObject(new ViewObject(obj)), position(p) {
}

optional<ViewObject> Trigger::getViewObject(WConstCreature) const {
  if (viewObject)
    return *viewObject;
  else
    return none;
}

void Trigger::onCreatureEnter(WCreature c) {}

void Trigger::fireDamage(double size) {}

bool Trigger::interceptsFlyingItem(WItem it) const { return false; }
void Trigger::onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir, VisionId) {}
void Trigger::tick() {}

namespace {

class Portal : public Trigger {
  public:

  WeakPointer<Portal> getOther(Position info) const {
    for (WTrigger t : info.getTriggers())
      if (auto ret = t.dynamicCast<Portal>())
        return ret;
    return nullptr;
  }

  WeakPointer<Portal> getOther() const {
    if (auto otherPortal = position.getGame()->getOtherPortal(position))
      return getOther(*otherPortal);
    return nullptr;
  }

  Portal(const ViewObject& obj, Position position) : Trigger(obj, position) {
    position.getGame()->registerPortal(position);
  }

  virtual void onCreatureEnter(WCreature c) override {
    if (!active) {
      active = true;
      return;
    }
    if (auto other = getOther()) {
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

  virtual bool interceptsFlyingItem(WItem it) const override {
    return getOther() && !Random.roll(5);
  }

  virtual void onInterceptFlyingItem(vector<PItem> it, const Attack& a, int remainingDist, Vec2 dir,
      VisionId vision) override {
    position.globalMessage(it[0]->getPluralTheNameAndVerb(it.size(), "disappears", "disappear") +
        " in the portal.");
    NOTNULL(getOther())->position.throwItem(std::move(it), a, remainingDist, dir, vision);
  }

  virtual void tick() override {
    position.getGame()->registerPortal(position);
    double time = position.getGame()->getGlobalTime();
    if (startTime == -1)
      startTime = time;
    if (time - startTime >= 30) {
      position.globalMessage("The portal disappears.");
      position.removeTrigger(this);
    }
  }

  SERIALIZE_ALL(SUBCLASS(Trigger), startTime, active);
  SERIALIZATION_CONSTRUCTOR(Portal);

  private:
  double SERIAL(startTime) = 1000000;
  bool SERIAL(active) = true;
};

}

PTrigger Trigger::getPortal(const ViewObject& obj, Position position) {
  return makeOwner<Portal>(obj, position);
}

namespace {

class MeteorShower : public Trigger {
  public:
  MeteorShower(WCreature c, double duration) : Trigger(c->getPosition()), creature(c),
      endTime(creature->getGlobalTime() + duration) {}

  virtual void tick() override {
    if (position.getGame()->getGlobalTime() >= endTime || (creature && creature->isDead())) {
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

  SERIALIZE_ALL(SUBCLASS(Trigger), endTime);
  SERIALIZATION_CONSTRUCTOR(MeteorShower);

  private:
  WCreature creature = nullptr; // Not serializing cause it might potentially cause a crash when saving a
      // single model in campaign mode.
  double SERIAL(endTime);
};

}

PTrigger Trigger::getMeteorShower(WCreature c, double duration) {
  return makeOwner<MeteorShower>(c, duration);
}

REGISTER_TYPE(Portal);
REGISTER_TYPE(MeteorShower);
