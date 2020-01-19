#pragma once

#include "util.h"
#include "corpse_info.h"
#include "item_class.h"

class ItemAttributes;
class ContentFactory;
class ItemTypeVariant;
class VictimEffect;

class ItemType {
  public:
  static ItemType legs(int damage);
  static ItemType claws(int damage);
  static ItemType beak(int damage);
  static ItemType fists(int damage);
  static ItemType fangs(int damage);
  static ItemType fangs(int damage, VictimEffect);
  static ItemType spellHit(int damage);
  static PItem corpse(const string& name, const string& rottenName, double weight, const ContentFactory* f, bool instantlyRotten = false,
      ItemClass = ItemClass::CORPSE,
      CorpseInfo corpseInfo = {UniqueEntity<Creature>::Id(), false, false, false});

  ItemType(const ItemTypeVariant&);
  ItemType();
  STRUCT_DECLARATIONS(ItemType)

  ItemType& setPrefixChance(double chance);

  template <class Archive>
  void serialize(Archive&, const unsigned int);

  PItem get(const ContentFactory*) const;
  vector<PItem> get(int, const ContentFactory*) const;

  HeapAllocated<ItemTypeVariant> SERIAL(type);

  private:
  ItemAttributes getAttributes(const ContentFactory*) const;
  double SERIAL(prefixChance) = 0.0;
};
