#include "stdafx.h"

#include "minion_equipment.h"
#include "item.h"
#include "creature.h"

ItemPredicate mushroomPredicate = [](const Item* it) {
  return contains({EffectType::STR_BONUS, EffectType::DEX_BONUS}, it->getEffectType());
};

Optional<MinionEquipment::EquipmentType> MinionEquipment::getEquipmentType(const Item* it) {
  if (it->getType() == ItemType::WEAPON)
    return MinionEquipment::WEAPON;
  if (it->getType() == ItemType::RANGED_WEAPON)
    return MinionEquipment::BOW;
  if (it->getType() == ItemType::AMMO)
    return MinionEquipment::ARROW;
  if (it->getType() == ItemType::ARMOR) {
    if (it->getEquipmentSlot() == EquipmentSlot::BODY_ARMOR)
      return MinionEquipment::BODY_ARMOR;
    if (it->getEquipmentSlot() == EquipmentSlot::HELMET)
      return MinionEquipment::HELMET;
    if (it->getEquipmentSlot() == EquipmentSlot::BOOTS)
      return MinionEquipment::BOOTS;
  }
  if (it->getEffectType() == EffectType::HEAL)
    return MinionEquipment::HEALING;
  if (mushroomPredicate(it))
    return MinionEquipment::MUSHROOM;
  return Nothing();
}

bool MinionEquipment::isItemUseful(const Item* it) const {
  return getEquipmentType(it);
}

bool MinionEquipment::needs(const Creature* c, const Item* it) {
  EquipmentType type = *getEquipmentType(it);
  return (type == ARROW && c->getEquipment().getItems(Item::typePredicate(ItemType::AMMO)).size() < 20 
          && c->hasSkill(Skill::archery)) ||
      ((c->canEquip(it) || (type == HEALING && !c->isNotLiving() &&
          c->getEquipment().getItems(Item::effectPredicate(EffectType::HEAL)).empty()) ||
      (type == MUSHROOM && c->getEquipment().getItems(mushroomPredicate).empty())));
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

