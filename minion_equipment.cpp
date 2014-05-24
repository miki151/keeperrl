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

#include "minion_equipment.h"
#include "item.h"
#include "creature.h"

static vector<EffectType> combatConsumables {
    EffectType::SPEED,
    EffectType::SLOW,
    EffectType::SLEEP,
    EffectType::POISON,
    EffectType::BLINDNESS,
    EffectType::INVISIBLE,
    EffectType::STR_BONUS,
    EffectType::DEX_BONUS,
};

template <class Archive>
void MinionEquipment::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(owners);
  CHECK_SERIAL;
}

SERIALIZABLE(MinionEquipment);

int MinionEquipment::getEquipmentLimit(EquipmentType type) {
  switch (type) {
    case MinionEquipment::ARMOR: return 1000;
    case MinionEquipment::COMBAT_ITEM:
    case MinionEquipment::HEALING: return 6;
    case MinionEquipment::ARCHERY: return 40;
  }
  FAIL << "wfe";
  return 0;
}

Optional<MinionEquipment::EquipmentType> MinionEquipment::getEquipmentType(const Item* it) {
  if (it->canEquip())
    return MinionEquipment::ARMOR;
  if (contains({ItemType::RANGED_WEAPON, ItemType::AMMO}, it->getType()))
    return MinionEquipment::ARCHERY;
  if (it->getEffectType() == EffectType::HEAL)
    return MinionEquipment::HEALING;
  if (contains(combatConsumables, it->getEffectType()))
    return MinionEquipment::COMBAT_ITEM;
  return Nothing();
}

bool MinionEquipment::isItemUseful(const Item* it) const {
  return getEquipmentType(it) || contains({ItemType::POTION, ItemType::SCROLL}, it->getType());
}

bool MinionEquipment::needs(const Creature* c, const Item* it, bool noLimit) {
  if (Optional<EquipmentType> type = getEquipmentType(it)) {
    int limit = noLimit ? 10000 : getEquipmentLimit(*type);
    if (c->getEquipment().getItems([&](const Item* it) { return getEquipmentType(it) == *type;}).size() >= limit)
      return false;
    return c->canEquip(it)
      || (type == ARCHERY && c->hasSkill(Skill::get(SkillId::ARCHERY)) && (c->canEquip(it) ||
              (it->getType() == ItemType::AMMO && c->getEquipment().getItem(EquipmentSlot::RANGED_WEAPON))))
      || (type == HEALING && !c->isNotLiving()) 
      || type == COMBAT_ITEM;
  } else
    return false;
}

const Creature* MinionEquipment::getOwner(const Item* it) const {
  if (owners.count(it->getUniqueId()))
    return owners.at(it->getUniqueId());
  else
    return nullptr;
}

void MinionEquipment::discard(const Item* it) {
  owners.erase(it->getUniqueId());
}

void MinionEquipment::own(const Creature* c, const Item* it) {
  owners[it->getUniqueId()] = c;
}

bool MinionEquipment::canTakeItem(const Creature* c, const Item* it) {
  if (const Creature* owner = getOwner(it)) {
    if (c == owner) {
      if (!needs(c, it)) {
        discard(it);
        return false;
      }
      return true;
    }
    if (owner->isDead())
      discard(it);
    else
      return false;
  }
  if (!getEquipmentType(it) || !c->isHumanoid())
    return false;
  return needs(c, it);
}

