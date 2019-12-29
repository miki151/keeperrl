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
#include "game_event.h"
#include "vision.h"
#include "draw_line.h"

SERIALIZE_DEF(RangedWeapon, damageAttr, projectileName, projectileViewId, maxDistance)
SERIALIZATION_CONSTRUCTOR_IMPL(RangedWeapon)

RangedWeapon::RangedWeapon(AttrType attr, const string& name, ViewId id, int dist)
    : damageAttr(attr), projectileName(name), projectileViewId(id), maxDistance(dist) {}

void RangedWeapon::fire(Creature* c, Position target) const {
  int damage = c->getAttr(damageAttr);
  Attack attack(c, Random.choose(AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH),
      AttackType::SHOOT, damage, damageAttr, {});
  const auto position = c->getPosition();
  auto vision = c->getVision().getId();
  Position lastPos;
  for (Vec2 offset : drawLine(position.getCoord(), target.getCoord()))
    if (offset != position.getCoord()) {
      Position pos(offset, target.getLevel());
      lastPos = pos;
      if (auto c = pos.getCreature()) {
        c->you(MsgType::HIT_THROWN_ITEM, "the " + projectileName);
        c->takeDamage(attack, true);
        break;
      }
      if (pos.stopsProjectiles(vision)) {
        pos.globalMessage("the " + projectileName + " hits the " + pos.getName());
        break;
      }
      if (*pos.dist8(position) >= maxDistance) {
        pos.globalMessage("the " + projectileName + " falls short.");
        break;
      }
    }
  c->getGame()->addEvent(EventInfo::Projectile{none, projectileViewId, position, lastPos, SoundId::SHOOT_BOW});
}

AttrType RangedWeapon::getDamageAttr() const {
  return damageAttr;
}

int RangedWeapon::getMaxDistance() const {
  return maxDistance;
}

#include "pretty_archive.h"
template
void RangedWeapon::serialize(PrettyInputArchive& ar1, unsigned);
