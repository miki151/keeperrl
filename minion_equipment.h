#ifndef _MINION_EQUIPMENT_H
#define _MINION_EQUIPMENT_H

#include "util.h"

class Creature;
class Item;

class MinionEquipment {
  public:
  bool needsItem(const Creature*, const Item*);

  bool isItemUseful(const Item*) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  enum EquipmentType { ARMOR, HEALING, ARCHERY, COMBAT_ITEM };

  bool needs(const Creature* c, const Item* it);
  static Optional<EquipmentType> getEquipmentType(const Item* it);

  map<pair<const Creature*, EquipmentType>, const Item*> equipmentMap;
  map<const Item*, const Creature*> owners;
};

#endif
