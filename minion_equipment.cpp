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
#include "creature_attributes.h"
#include "effect_type.h"
#include "body.h"
#include "item_class.h"
#include "corpse_info.h"

static bool isCombatConsumable(EffectType type) {
  if (type.getId() != EffectId::LASTING)
    return false;
  switch (type.get<LastingEffect>()) {
    case LastingEffect::SPEED:
    case LastingEffect::SLOWED:
    case LastingEffect::SLEEP:
    case LastingEffect::POISON:
    case LastingEffect::BLIND:
    case LastingEffect::INVISIBLE:
    case LastingEffect::STR_BONUS:
    case LastingEffect::DEX_BONUS:
    case LastingEffect::POISON_RESISTANT:
      return true;
    default:
      return false;
  }
}

template <class Archive>
void MinionEquipment::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, owners, locked, myItems);
}

SERIALIZABLE(MinionEquipment);

optional<int> MinionEquipment::getEquipmentLimit(EquipmentType type) const {
  switch (type) {
    case MinionEquipment::COMBAT_ITEM:
    case MinionEquipment::HEALING:
      return 6;
    case MinionEquipment::ARCHERY:
      return 40;
    default:
      return none;
  }
}

optional<MinionEquipment::EquipmentType> MinionEquipment::getEquipmentType(const Item* it) {
  if (it->canEquip())
    return MinionEquipment::ARMOR;
  if (it->getClass() == ItemClass::AMMO)
    return MinionEquipment::ARCHERY;
  if (auto& effect = it->getEffectType()) {
    if (effect->getId() == EffectId::HEAL)
      return MinionEquipment::HEALING;
    if (isCombatConsumable(*effect))
      return MinionEquipment::COMBAT_ITEM;
  }
  return none;
}

bool MinionEquipment::isItemUseful(const Item* it) {
  static EnumSet<ItemClass> usefulItems {ItemClass::GOLD, ItemClass::POTION, ItemClass::SCROLL};
  return getEquipmentType(it) || usefulItems.contains(it->getClass())
      || (it->getClass() == ItemClass::FOOD && !it->getCorpseInfo());
}

bool MinionEquipment::needsItem(const Creature* c, const Item* it, bool noLimit) const {
  if (optional<EquipmentType> type = getEquipmentType(it)) {
    if (!noLimit) {
      auto itemValue = getItemValue(it);
      if (auto limit = getEquipmentLimit(*type)) {
        auto pred = [=](const Item* ownedItem) {
          return getEquipmentType(ownedItem) == *type &&
              (getItemValue(ownedItem) >= itemValue || isLocked(c, ownedItem->getUniqueId())) &&
              ownedItem != it;
        };
        if (getItemsOwnedBy(c, pred).size() >= *limit)
          return false;
      }
      if (it->canEquip()) {
        auto slot = it->getEquipmentSlot();
        int limit = c->getEquipment().getMaxItems(slot);
        auto pred = [=](const Item* ownedItem) {
          return ownedItem->canEquip() &&
              ownedItem->getEquipmentSlot() == slot &&
              (getItemValue(ownedItem) >= itemValue || isLocked(c, ownedItem->getUniqueId())) &&
              ownedItem != it;
        };
        if (getItemsOwnedBy(c, pred).size() >= limit)
          return false;
      }
    }
    return (c->canEquipIfEmptySlot(it) && (isItemAppropriate(c, it) || noLimit))
      || (type == ARCHERY && !getItemsOwnedBy(c, Item::isRangedWeaponPredicate()).empty())
      || (type == HEALING && c->getBody().hasHealth()) 
      || type == COMBAT_ITEM;
  } else
    return false;
}

optional<Creature::Id> MinionEquipment::getOwner(const Item* it) const {
  if (auto creature = owners.getMaybe(it))
    return *creature;
  else
    return none;
}

bool MinionEquipment::isOwner(const Item* it, const Creature* c) const {
  return getOwner(it) == c->getUniqueId();
}

void MinionEquipment::updateOwners(const vector<Creature*>& creatures) {
  auto oldItemMap = myItems;
  myItems.clear();
  owners.clear();
  for (auto c : creatures)
    if (auto items = oldItemMap.getMaybe(c))
      for (auto wItem : *items)
        if (auto item = wItem.lock())
          own(c, item.get());
  for (auto c : creatures)
    for (auto item : getItemsOwnedBy(c))
      if (!needsItem(c, item))
        discard(item);
}

void MinionEquipment::updateItems(const vector<Item*>& items) {
  auto oldOwners = owners;
  myItems.clear();
  owners.clear();
  for (auto it : items)
    if (auto owner = oldOwners.getMaybe(it)) {
      myItems.getOrInit(*owner).push_back(it->shared_from_this());
      owners.set(it->getUniqueId(), *owner);
    }
}

const static vector<WItem> emptyItems;

vector<Item*> MinionEquipment::getItemsOwnedBy(const Creature* c, ItemPredicate predicate) const {
  vector<Item*> ret;
  for (auto& wItem : myItems.getOrElse(c, emptyItems))
    if (auto item = wItem.lock())
      if (!predicate || predicate(item.get()))
        ret.push_back(item.get());
  return ret;
}

void MinionEquipment::discard(const Item* it) {
  discard(it->getUniqueId());
}

void MinionEquipment::discard(UniqueEntity<Item>::Id id) {
  if (auto owner = owners.getMaybe(id)) {
    locked.erase(make_pair(*owner, id));
    owners.erase(id);
    auto& items = myItems.getOrFail(*owner);
    for (int i : All(items))
      if (auto item = items[i].lock())
        if (item->getUniqueId() == id) {
          removeIndex(items, i);
          break;
        }
  }
}

void MinionEquipment::sortByEquipmentValue(vector<Item*>& items) const {
  sort(items.begin(), items.end(), [this](const Item* it1, const Item* it2) {
      int diff = getItemValue(it1) - getItemValue(it2);
      if (diff == 0)
        return it1->getUniqueId() < it2->getUniqueId();
      else
        return diff > 0;
    });
}

void MinionEquipment::own(const Creature* c, Item* it) {
  if (it->canEquip()) {
    auto slot = it->getEquipmentSlot();
    vector<Item*> contesting;
    int slotSize = c->getEquipment().getMaxItems(slot);
    for (auto& wItem : myItems.getOrElse(c, emptyItems))
      if (auto item = wItem.lock())
        if (item->canEquip() && item->getEquipmentSlot() == slot) {
          if (!isLocked(c, item->getUniqueId()))
            contesting.push_back(item.get());
          else
            --slotSize;
        }
    if (contesting.size() >= slotSize) {
      sortByEquipmentValue(contesting);
      for (int i = slotSize - 1; i < contesting.size(); ++i)
        discard(contesting[i]);
    }
  }
  discard(it);
  owners.set(it, c->getUniqueId());
  myItems.getOrInit(c).push_back(it->shared_from_this());
}

Item* MinionEquipment::getWorstItem(const Creature* c, vector<Item*> items) const {
  Item* ret = nullptr;
  for (Item* it : items)
    if (!isLocked(c, it->getUniqueId()) &&
        (ret == nullptr || getItemValue(it) < getItemValue(ret)))
      ret = it;
  return ret;
}

void MinionEquipment::autoAssign(const Creature* creature, vector<Item*> possibleItems) {
  map<EquipmentSlot, vector<Item*>> slots;
  for (Item* it : getItemsOwnedBy(creature))
    if (it->canEquip()) {
      EquipmentSlot slot = it->getEquipmentSlot();
      slots[slot].push_back(it);
    }
  sortByEquipmentValue(possibleItems);
  for (Item* it : possibleItems)
    if (!getOwner(it) && needsItem(creature, it)) {
      if (!it->canEquip()) {
        own(creature, it);
        if (it->getClass() != ItemClass::AMMO)
          break;
        else
          continue;
      }
      Item* replacedItem = getWorstItem(creature, slots[it->getEquipmentSlot()]);
      int slotSize = creature->getEquipment().getMaxItems(it->getEquipmentSlot());
      int numInSlot = slots[it->getEquipmentSlot()].size();
      if (numInSlot < slotSize ||
          (replacedItem && getItemValue(replacedItem) < getItemValue(it))) {
        if (numInSlot == slotSize) {
          discard(replacedItem);
          removeElement(slots[it->getEquipmentSlot()], replacedItem);
        }
        own(creature, it);
        slots[it->getEquipmentSlot()].push_back(it);
        break;
      }
  }
}

bool MinionEquipment::isItemAppropriate(const Creature* c, const Item* it) const {
  return c->isEquipmentAppropriate(it);
}

int MinionEquipment::getItemValue(const Item* it) const {
  return it->getModifier(ModifierType::ACCURACY) + it->getModifier(ModifierType::DAMAGE)
    + it->getModifier(ModifierType::DEFENSE) + it->getModifier(ModifierType::FIRED_ACCURACY)
    + it->getModifier(ModifierType::FIRED_DAMAGE);
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

