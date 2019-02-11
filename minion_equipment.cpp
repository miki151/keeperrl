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
#include "creature_attributes.h"
#include "effect.h"
#include "body.h"
#include "item_class.h"
#include "corpse_info.h"
#include "weapon_info.h"

static bool isCombatConsumable(Effect type) {
  return type.visit(
      [&](const Effect::Teleport&) { return true; },
      [&](const Effect::Lasting& e) {
        switch (e.lastingEffect) {
          case LastingEffect::SPEED:
          case LastingEffect::SLOWED:
          case LastingEffect::SLEEP:
          case LastingEffect::POISON:
          case LastingEffect::BLIND:
          case LastingEffect::INVISIBLE:
          case LastingEffect::DAM_BONUS:
          case LastingEffect::DEF_BONUS:
          case LastingEffect::POISON_RESISTANT:
          case LastingEffect::MELEE_RESISTANCE:
          case LastingEffect::RANGED_RESISTANCE:
          case LastingEffect::MAGIC_RESISTANCE:
          case LastingEffect::REGENERATION:
            return true;
          default:
            return false;
        }
      },
      [&](const auto&) { return false; }
  );
}

template <class Archive>
void MinionEquipment::serialize(Archive& ar, const unsigned int) {
  ar(owners, locked, myItems);
}

SERIALIZABLE(MinionEquipment);

optional<int> MinionEquipment::getEquipmentLimit(EquipmentType type) const {
  switch (type) {
    case MinionEquipment::COMBAT_ITEM:
    case MinionEquipment::HEALING:
      return 6;
    case MinionEquipment::TORCH:
      return 1;
    default:
      return none;
  }
}

optional<MinionEquipment::EquipmentType> MinionEquipment::getEquipmentType(const Item* it) {
  if (it->canEquip())
    return MinionEquipment::ARMOR;
  if (auto& effect = it->getEffect()) {
    if (effect->isType<Effect::Heal>())
      return MinionEquipment::HEALING;
    if (isCombatConsumable(*effect))
      return MinionEquipment::COMBAT_ITEM;
  }
  if (it->getOwnedEffect() == LastingEffect::LIGHT_SOURCE)
    return MinionEquipment::TORCH;
  return none;
}

bool MinionEquipment::isItemUseful(const Item* it) {
  static EnumSet<ItemClass> usefulItems {ItemClass::GOLD, ItemClass::POTION, ItemClass::SCROLL};
  return getEquipmentType(it) || usefulItems.contains(it->getClass())
      || (it->getClass() == ItemClass::FOOD && !it->getCorpseInfo());
}

bool MinionEquipment::needsItem(const Creature* c, const Item* it, bool noLimit) const {
  PROFILE;
  if (optional<EquipmentType> type = getEquipmentType(it)) {
    if (!noLimit) {
      auto itemValue = getItemValue(c, it);
      if (auto limit = getEquipmentLimit(*type)) {
        auto pred = [=](const Item* ownedItem) {
          return getEquipmentType(ownedItem) == *type &&
              (getItemValue(c, ownedItem) >= itemValue || isLocked(c, ownedItem->getUniqueId())) &&
              ownedItem != it;
        };
        if (getItemsOwnedBy(c, pred).size() >= *limit)
          return false;
      }
      if (it->canEquip()) {
        auto slot = it->getEquipmentSlot();
        int limit = c->getEquipment().getMaxItems(slot, c->getBody());
        auto pred = [=](const Item* ownedItem) {
          return ownedItem->canEquip() &&
              ownedItem->getEquipmentSlot() == slot &&
              (getItemValue(c, ownedItem) >= itemValue || isLocked(c, ownedItem->getUniqueId())) &&
              ownedItem != it;
        };
        if (getItemsOwnedBy(c, pred).size() >= limit)
          return false;
      }
    }
    return (c->canEquipIfEmptySlot(it))
      || (type == HEALING && c->getBody().hasHealth()) 
      || type == COMBAT_ITEM || type == TORCH;
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

const static vector<WeakPointer<Item>> emptyItems;

void MinionEquipment::updateOwners(const vector<Creature*>& creatures) {
  auto oldItemMap = myItems;
  myItems.clear();
  owners.clear();
  for (auto c : creatures)
    for (auto item : oldItemMap.getOrElse(c, emptyItems))
      if (item) {
        owners.set(item, c->getUniqueId());
        myItems.getOrInit(c).push_back(item);
      }
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
      myItems.getOrInit(*owner).push_back(it);
      owners.set(it->getUniqueId(), *owner);
    }
}

vector<Item*> MinionEquipment::getItemsOwnedBy(const Creature* c, ItemPredicate predicate) const {
  PROFILE;
  vector<Item*> ret;
  for (auto& item : myItems.getOrElse(c, emptyItems))
    if (item)
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
      if (items[i])
        if (items[i]->getUniqueId() == id) {
          items.removeIndex(i);
          break;
        }
  }
}

void MinionEquipment::sortByEquipmentValue(const Creature* c, vector<Item*>& items) const {
  PROFILE;
  vector<int> values;
  vector<int> indexes;
  for (auto& item : items) {
    values.push_back(getItemValue(c, item));
    indexes.push_back(indexes.size());
  }
  sort(indexes.begin(), indexes.end(), [&](int index1, int index2) {
      int diff = values[index1] - values[index2];
      if (diff == 0)
        return items[index1]->getUniqueId() < items[index2]->getUniqueId();
      else
        return diff > 0;
    });
  vector<Item*> ret;
  for (auto& index : indexes)
    ret.push_back(items[index]);
  items = ret;
}

bool MinionEquipment::tryToOwn(const Creature* c, Item* it) {
  if (it->canEquip()) {
    auto slot = it->getEquipmentSlot();
    vector<Item*> contesting;
    int slotSize = c->getEquipment().getMaxItems(slot, c->getBody());
    for (auto& item : myItems.getOrElse(c, emptyItems))
      if (item)
        if (item->canEquip() && item->getEquipmentSlot() == slot) {
          if (!isLocked(c, item->getUniqueId()))
            contesting.push_back(item.get());
          else if (--slotSize <= 0)
            return false;
        }
    if (contesting.size() >= slotSize) {
      sortByEquipmentValue(c, contesting);
      for (int i = slotSize - 1; i < contesting.size(); ++i)
        discard(contesting[i]);
    }
  }
  discard(it);
  owners.set(it, c->getUniqueId());
  myItems.getOrInit(c).push_back(it);
  return true;
}

Item* MinionEquipment::getWorstItem(const Creature* c, vector<Item*> items) const {
  PROFILE;
  Item* ret = nullptr;
  for (Item* it : items)
    if (!isLocked(c, it->getUniqueId()) &&
        (!ret || getItemValue(c, it) < getItemValue(c, ret)))
      ret = it;
  return ret;
}

static bool canAutoAssignItem(const Item* item) {
  if (auto effect = item->getWeaponInfo().attackerEffect)
    if (effect->isType<Effect::Suicide>())
      return false;
  return true;
}

void MinionEquipment::autoAssign(const Creature* creature, vector<Item*> possibleItems) {
  PROFILE;
  map<EquipmentSlot, vector<Item*>> slots;
  for (Item* it : getItemsOwnedBy(creature))
    if (it->canEquip()) {
      EquipmentSlot slot = it->getEquipmentSlot();
      slots[slot].push_back(it);
    }
  sortByEquipmentValue(creature, possibleItems);
  for (Item* it : possibleItems)
    if (!getOwner(it) && needsItem(creature, it) && canAutoAssignItem(it)) {
      if (!it->canEquip()) {
        CHECK(tryToOwn(creature, it));
        continue;
      }
      Item* replacedItem = getWorstItem(creature, slots[it->getEquipmentSlot()]);
      int slotSize = creature->getEquipment().getMaxItems(it->getEquipmentSlot(), creature->getBody());
      int numInSlot = slots[it->getEquipmentSlot()].size();
      if (numInSlot < slotSize ||
          (replacedItem && getItemValue(creature, replacedItem) < getItemValue(creature, it))) {
        if (numInSlot == slotSize) {
          discard(replacedItem);
          slots[it->getEquipmentSlot()].removeElement(replacedItem);
        }
        CHECK(tryToOwn(creature, it));
        slots[it->getEquipmentSlot()].push_back(it);
        break;
      }
  }
}

int MinionEquipment::getItemValue(const Creature* c, const Item* it) const {
  PROFILE;
  int sum = 0;
  for (auto attr : ENUM_ALL(AttrType))
    switch (attr) {
      case AttrType::DAMAGE:
      case AttrType::SPELL_DAMAGE:
        if (it->getClass() != ItemClass::WEAPON || attr == it->getWeaponInfo().meleeAttackAttr)
          sum += c->getAttributes().getRawAttr(attr) + it->getModifier(attr);
        break;
      default:
        sum += it->getModifier(attr);
        break;
    }
  return sum;
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

