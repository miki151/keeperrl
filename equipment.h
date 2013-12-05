#ifndef _EQUIPMENT_H
#define _EQUIPMENT_H

#include "inventory.h"
#include "enums.h"


class Equipment : public Inventory {
  public:
  Item* getItem(EquipmentSlot slot) const;
  bool isEquiped(const Item*) const;
  EquipmentSlot getSlot(const Item*) const;
  void equip(Item*, EquipmentSlot);
  void unequip(EquipmentSlot slot);
  PItem removeItem(Item*);
  vector<PItem> removeItems(const vector<Item*>&);
  vector<PItem> removeAllItems();

  private:
  map<EquipmentSlot, Item*> items;
};

#endif
