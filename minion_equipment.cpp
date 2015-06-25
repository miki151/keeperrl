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
#include "effect.h"
#include "equipment.h"
#include "modifier_type.h"

static vector<EffectType> combatConsumables {
    EffectType(EffectId::LASTING, LastingEffect::SPEED),
    EffectType(EffectId::LASTING, LastingEffect::SLOWED),
    EffectType(EffectId::LASTING, LastingEffect::SLEEP),
    EffectType(EffectId::LASTING, LastingEffect::POISON),
    EffectType(EffectId::LASTING, LastingEffect::BLIND),
    EffectType(EffectId::LASTING, LastingEffect::INVISIBLE),
    EffectType(EffectId::LASTING, LastingEffect::STR_BONUS),
    EffectType(EffectId::LASTING, LastingEffect::DEX_BONUS),
    EffectType(EffectId::LASTING, LastingEffect::POISON_RESISTANT),
};

template <class Archive>
void MinionEquipment::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(owners) & SVAR(locked);
}

SERIALIZABLE(MinionEquipment);

int MinionEquipment::getEquipmentLimit(EquipmentType type) const {
  switch (type) {
    case MinionEquipment::ARMOR: return 1000;
    case MinionEquipment::COMBAT_ITEM:
    case MinionEquipment::HEALING: return 6;
    case MinionEquipment::ARCHERY: return 40;
  }
}

optional<MinionEquipment::EquipmentType> MinionEquipment::getEquipmentType(const Item* it) {
  if (it->canEquip())
    return MinionEquipment::ARMOR;
  if (contains({ItemClass::RANGED_WEAPON, ItemClass::AMMO}, it->getClass()))
    return MinionEquipment::ARCHERY;
  if (it->getEffectType() == EffectType(EffectId::HEAL))
    return MinionEquipment::HEALING;
  if (contains(combatConsumables, it->getEffectType()))
    return MinionEquipment::COMBAT_ITEM;
  return none;
}

bool MinionEquipment::isItemUseful(const Item* it) {
  static EnumSet<ItemClass> usefulItems {ItemClass::GOLD, ItemClass::POTION, ItemClass::SCROLL};
  return getEquipmentType(it) || usefulItems[it->getClass()]
      || (it->getClass() == ItemClass::FOOD && !it->getCorpseInfo());
}

bool MinionEquipment::needs(const Creature* c, const Item* it, bool noLimit, bool replacement) const {
  if (optional<EquipmentType> type = getEquipmentType(it)) {
    int limit = noLimit ? 10000 : getEquipmentLimit(*type);
    if (c->getEquipment().getItems([&](const Item* it) { return getEquipmentType(it) == *type;}).size() >= limit)
      return false;
    return ((c->canEquip(it) || (replacement && c->canEquipIfEmptySlot(it))) && (isItemAppropriate(c, it) || noLimit))
      || (type == ARCHERY && (c->canEquip(it) ||
        (it->getClass() == ItemClass::AMMO && !c->getEquipment().getItem(EquipmentSlot::RANGED_WEAPON).empty())))
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

void MinionEquipment::updateOwners(const vector<Item*> items) {
  for (const Item* item : items)
    if (owners.count(item->getUniqueId())) {
      const Creature* c = owners.at(item->getUniqueId());
      if (c->isDead() || !needs(c, item, true, true))
        discard(item);
    }
}

void MinionEquipment::discard(const Item* it) {
  discard(it->getUniqueId());
}

void MinionEquipment::discard(UniqueEntity<Item>::Id id) {
  owners.erase(id);
}

void MinionEquipment::own(const Creature* c, const Item* it) {
  owners[it->getUniqueId()] = c;
}

bool MinionEquipment::isItemAppropriate(const Creature* c, const Item* it) const {
  return c->isEquipmentAppropriate(it);
}

int MinionEquipment::getItemValue(const Item* it) {
  return it->getModifier(ModifierType::ACCURACY) + it->getModifier(ModifierType::DAMAGE)
    + it->getModifier(ModifierType::DEFENSE);
}

void MinionEquipment::setLocked(const Creature* c, UniqueEntity<Item>::Id it, bool lock) {
  if (lock)
    locked.insert(make_pair(c->getUniqueId(), it));
  else
    locked.erase(make_pair(c->getUniqueId(), it));
}

bool MinionEquipment::isLocked(const Creature* c, UniqueEntity<Item>::Id it) const {
  return locked.count(make_pair(c->getUniqueId(), it));
}

