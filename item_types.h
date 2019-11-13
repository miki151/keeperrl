#pragma once

#include "util.h"
#include "weapon_info.h"
#include "item_prefix.h"
#include "item_upgrade_info.h"
#include "view_id.h"
#include "tech_id.h"
#include "custom_item_id.h"
#include "furniture_type.h"

#define ITEM_TYPE_INTERFACE\
  ItemAttributes getAttributes(const ContentFactory*) const

#define SIMPLE_ITEM(Name) \
  struct Name { \
    ITEM_TYPE_INTERFACE;\
    COMPARE_ALL()\
  }

namespace ItemTypes {
struct Intrinsic {
  ViewId viewId;
  string name;
  int damage;
  WeaponInfo weaponInfo;
  COMPARE_ALL(viewId, name, damage, weaponInfo)
  ITEM_TYPE_INTERFACE;
};
struct Scroll {
  Effect effect;
  COMPARE_ALL(effect)
  ITEM_TYPE_INTERFACE;
};
struct Potion {
  Effect effect;
  COMPARE_ALL(effect)
  ITEM_TYPE_INTERFACE;
};
struct Mushroom {
  Effect effect;
  COMPARE_ALL(effect)
  ITEM_TYPE_INTERFACE;
};
struct Amulet {
  LastingEffect lastingEffect;
  COMPARE_ALL(lastingEffect)
  ITEM_TYPE_INTERFACE;
};
struct Ring {
  LastingEffect lastingEffect;
  COMPARE_ALL(lastingEffect)
  ITEM_TYPE_INTERFACE;
};
struct Glyph {
  ItemUpgradeInfo rune;
  COMPARE_ALL(rune)
  ITEM_TYPE_INTERFACE;
};
struct TechBook {
  TechId techId;
  COMPARE_ALL(techId)
  ITEM_TYPE_INTERFACE;
};
struct TrapItem {
  FurnitureType trapType;
  string trapName;
  COMPARE_ALL(trapType, trapName)
  ITEM_TYPE_INTERFACE;
};
SIMPLE_ITEM(FireScroll);
SIMPLE_ITEM(Poem);
struct EventPoem {
  string eventName;
  COMPARE_ALL(eventName)
  ITEM_TYPE_INTERFACE;
};
using Simple = CustomItemId;
MAKE_VARIANT(ItemTypeVariant, Scroll, Potion, Mushroom, Amulet, Ring, TechBook, TrapItem,  Intrinsic, Glyph, Simple, FireScroll, Poem, EventPoem);

}

class ItemTypeVariant : public ItemTypes::ItemTypeVariant {
  public:
  using ItemTypes::ItemTypeVariant::ItemTypeVariant;
};
