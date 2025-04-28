#pragma once

#include "util.h"
#include "corpse_info.h"
#include "item_class.h"

class ContentFactory;
class ItemTypeVariant;

namespace ItemPrefixes {
class VictimEffect;
}

class ItemType {
  public:
  static ItemType legs(int damage);
  static ItemType claws(int damage);
  static ItemType beak(int damage);
  static ItemType fists(int damage);
  static ItemType fangs(int damage);
  static ItemType fangs(int damage, ItemPrefixes::VictimEffect);
  static ItemType spellHit(int damage);
  static PItem corpse(const string& name, const string& rottenName, double weight, const ContentFactory* f,
      bool instantlyRotten = false, ItemClass = ItemClass::CORPSE,
      CorpseInfo corpseInfo = {UniqueEntity<Creature>::Id(), false, false, false}, optional<string> ingredient = none);
  static PItem severedLimb(const string& creatureName, BodyPart, double weight, ItemClass, const ContentFactory*);

  ItemType(const ItemTypeVariant&);
  ItemType();
  STRUCT_DECLARATIONS(ItemType)

  template <class Archive>
  void serialize(Archive&, const unsigned int);

  PItem get(const ContentFactory*) const;
  vector<PItem> get(int, const ContentFactory*) const;
  SItemAttributes getAttributes(const ContentFactory*) const;
  ItemType setPrefixChance(double chance)&&;

  HeapAllocated<ItemTypeVariant> SERIAL(type);
};

static_assert(std::is_nothrow_move_constructible<ItemType>::value, "T should be noexcept MoveConstructible");
