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

int MinionEquipment::getEquipmentLimit(EquipmentType type) const {
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

bool MinionEquipment::needs(const Creature* c, const Item* it, bool noLimit, bool replacement) const {
  if (Optional<EquipmentType> type = getEquipmentType(it)) {
    int limit = noLimit ? 10000 : getEquipmentLimit(*type);
    if (c->getEquipment().getItems([&](const Item* it) { return getEquipmentType(it) == *type;}).size() >= limit)
      return false;
    return ((c->canEquip(it) || (replacement && c->canEquipIfEmptySlot(it))) && (isItemAppropriate(c, it) || noLimit))
      || (type == ARCHERY && c->hasSkill(Skill::get(SkillId::ARCHERY)) && (c->canEquip(it) ||
        (it->getType() == ItemType::AMMO && getOnlyElement(c->getEquipment().getItem(EquipmentSlot::RANGED_WEAPON)))))
      || (type == HEALING && !c->isNotLiving()) 
      || type == COMBAT_ITEM;
  } else
    return false;
}

const Creature* MinionEquipment::getOwner(const Item* it) const {
  if (owners.count(it->getUniqueId())) {
    const Creature* c = owners.at(it->getUniqueId());
    if (!c->isDead() && needs(c, it, true, true))
      return c;
  }
  return nullptr;
}

void MinionEquipment::discard(const Item* it) {
  owners.erase(it->getUniqueId());
}

void MinionEquipment::own(const Creature* c, const Item* it) {
  owners[it->getUniqueId()] = c;
}

bool MinionEquipment::isItemAppropriate(const Creature* c, const Item* it) const {
  return it->getType() != ItemType::WEAPON || c->getAttr(AttrType::STRENGTH) >= it->getMinStrength();
}

int MinionEquipment::getItemValue(const Item* it) const {
  return it->getModifier(AttrType::TO_HIT) + it->getModifier(AttrType::DAMAGE)
    + it->getModifier(AttrType::DEFENSE);
}

