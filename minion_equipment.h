#ifndef _MINION_EQUIPMENT_H
#define _MINION_EQUIPMENT_H

#include "util.h"
#include "unique_entity.h"

class Creature;
class Item;

class MinionEquipment {
  public:
  bool canTakeItem(const Creature*, const Item*);

  bool isItemUseful(const Item*) const;
  bool needs(const Creature* c, const Item* it);
  const Creature* getOwner(const Item*) const;
  void own(const Creature*, const Item*);
  void discard(const Item*);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  SERIAL_CHECKER;

  private:
  enum EquipmentType { ARMOR, HEALING, ARCHERY, COMBAT_ITEM };

  static Optional<EquipmentType> getEquipmentType(const Item* it);
  int getEquipmentLimit(EquipmentType type);

  map<UniqueId, const Creature*> SERIAL(owners);
};

#endif
