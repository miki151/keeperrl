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

#include "ranged_weapon.h"
#include "creature.h"
#include "level.h"
#include "attack.h"
#include "view.h"
#include "game.h"
#include "sound.h"
#include "attack_level.h"
#include "attack_type.h"
#include "skill.h"
#include "creature_attributes.h"
#include "player_message.h"
#include "view_id.h"
#include "event_listener.h"

SERIALIZE_DEF(RangedWeapon, damageAttr, projectileName, projectileViewId)
SERIALIZATION_CONSTRUCTOR_IMPL(RangedWeapon);

RangedWeapon::RangedWeapon(AttrType attr, const string& name, ViewId id)
    : damageAttr(attr), projectileName(name), projectileViewId(id) {}

void RangedWeapon::fire(WCreature c, Vec2 dir) const {
  CHECK(dir.length8() == 1);
  c->getGame()->getView()->addSound(SoundId::SHOOT_BOW);
  int attackVariance = 3;
  int damage = c->getAttr(damageAttr) + Random.get(-attackVariance, attackVariance);
  Attack attack(c, Random.choose(AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH),
      AttackType::SHOOT, damage, damageAttr, none);
  const auto position = c->getPosition();
  auto vision = c->getVision();
  Position lastPos;
  for (Position pos = position.plus(dir);; pos = pos.plus(dir)) {
    lastPos = pos;
    if (auto c = pos.getCreature()) {
      c->you(MsgType::HIT_THROWN_ITEM, "the " + projectileName);
      c->takeDamage(attack);
      break;
    }
    if (!pos.canSeeThru(vision)) {
      pos.globalMessage("the " + projectileName + " hits the " + pos.getName());
      break;
    }
  }
  c->getGame()->addEvent({EventId::PROJECTILE, EventInfo::Projectile{projectileViewId, position, lastPos}});
}

AttrType RangedWeapon::getDamageAttr() const {
  return damageAttr;
}
