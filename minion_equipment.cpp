#include "stdafx.h"

#include "minion_equipment.h"
#include "item.h"
#include "creature.h"

static vector<EffectType> combatConsumables {
    EffectType::SPEED,
    EffectType::SLOW,
    EffectType::SLEEP,
    EffectType::BLINDNESS,
    EffectType::INVISIBLE,
    EffectType::STR_BONUS,
    EffectType::DEX_BONUS,
};

Optional<MinionEquipment::EquipmentType> MinionEquipment::getEquipmentType(const Item* it) {
  if (contains({ItemType::WEAPON, ItemType::ARMOR}, it->getType()))
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

bool MinionEquipment::needs(const Creature* c, const Item* it) {
  EquipmentType type = *getEquipmentType(it);
  return c->canEquip(it) || (type == ARCHERY && c->hasSkill(Skill::archery))
      || (type == HEALING && !c->isNotLiving()) 
      || type == COMBAT_ITEM;
}

bool MinionEquipment::needsItem(const Creature* c, const Item* it) {
  if (owners.count(it)) {
    if (owners.at(it) == c)
      return true;
    if (owners.at(it)->isDead())
      owners.erase(it);
    else
      return false;
  }
  if (!getEquipmentType(it) || !c->isHumanoid())
    return false;
  if (needs(c, it)) {
    owners[it] = c;
    return true;
  } else
    return false;
}

