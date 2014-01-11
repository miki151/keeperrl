#ifndef _MINION_EQUIPMENT_H
#define _MINION_EQUIPMENT_H

class MinionEquipment {
  public:
  bool needsItem(const Creature*, const Item*);

  bool isItemUseful(const Item*);

  private:
  enum EquipmentType { WEAPON, BODY_ARMOR, HELMET, BOOTS, HEALING, BOW, ARROW, MUSHROOM };

  bool needs(const Creature* c, const Item* it);
  static Optional<EquipmentType> getEquipmentType(const Item* it);

  map<pair<const Creature*, EquipmentType>, const Item*> equipmentMap;
  map<const Item*, const Creature*> owners;
};

#endif
