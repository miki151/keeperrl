#include "stdafx.h"

Optional<MinionEquipment::EquipmentType> MinionEquipment::getEquipmentType(const Item* it) {
  if (it->getType() == ItemType::WEAPON)
    return MinionEquipment::WEAPON;
  if (it->getType() == ItemType::ARMOR) {
    if (it->getEquipmentSlot() == EquipmentSlot::BODY_ARMOR)
      return MinionEquipment::BODY_ARMOR;
    if (it->getEquipmentSlot() == EquipmentSlot::HELMET)
      return MinionEquipment::HELMET;
  }
  if (it->getType() == ItemType::TOOL && it->getEffectType() == EffectType::HEAL)
    return MinionEquipment::FIRST_AID_KIT;
  return Nothing();
}

bool MinionEquipment::isItemUseful(const Item* it) {
  return getEquipmentType(it);
}

bool MinionEquipment::needs(const Creature* c, const Item* it) {
  EquipmentType type = *getEquipmentType(it);
  return (!equipmentMap.count(make_pair(c, type)) &&
      (c->canEquip(it) || (type == MinionEquipment::FIRST_AID_KIT && !c->isUndead() &&
          c->getEquipment().getItems(Item::effectPredicate(EffectType::HEAL)).empty())));
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
    equipmentMap[make_pair(c, *getEquipmentType(it))] = it;
    return true;
  } else
    return false;
}

