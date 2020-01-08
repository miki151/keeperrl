#pragma once

#include "util.h"
#include "weapon_info.h"
#include "item_prefix.h"
#include "item_upgrade_info.h"
#include "view_id.h"
#include "tech_id.h"
#include "custom_item_id.h"
#include "furniture_type.h"
#include "creature_id.h"

#define ITEM_TYPE_INTERFACE\
  ItemAttributes getAttributes(const ContentFactory*) const

#define SIMPLE_ITEM(Name) \
  struct Name : public EmptyStruct<Name> { \
    ITEM_TYPE_INTERFACE;\
  }

namespace ItemTypes {
struct Intrinsic {
  ViewId SERIAL(viewId);
  string SERIAL(name);
  int SERIAL(damage);
  WeaponInfo SERIAL(weaponInfo);
  SERIALIZE_ALL(viewId, name, damage, weaponInfo)
  ITEM_TYPE_INTERFACE;
};
struct Scroll {
  Effect SERIAL(effect);
  SERIALIZE_ALL(effect)
  ITEM_TYPE_INTERFACE;
};
struct Potion {
  Effect SERIAL(effect);
  SERIALIZE_ALL(effect)
  ITEM_TYPE_INTERFACE;
};
struct Mushroom {
  Effect SERIAL(effect);
  SERIALIZE_ALL(effect)
  ITEM_TYPE_INTERFACE;
};
struct Amulet {
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
  ITEM_TYPE_INTERFACE;
};
struct Ring {
  LastingEffect SERIAL(lastingEffect);
  SERIALIZE_ALL(lastingEffect)
  ITEM_TYPE_INTERFACE;
};
struct Glyph {
  ItemUpgradeInfo SERIAL(rune);
  SERIALIZE_ALL(rune)
  ITEM_TYPE_INTERFACE;
};
struct TechBook {
  TechId SERIAL(techId);
  SERIALIZE_ALL(techId)
  ITEM_TYPE_INTERFACE;
};
struct TrapItem {
  FurnitureType SERIAL(trapType);
  string SERIAL(trapName);
  SERIALIZE_ALL(trapType, trapName)
  ITEM_TYPE_INTERFACE;
};
SIMPLE_ITEM(FireScroll);
SIMPLE_ITEM(Poem);
struct EventPoem {
  string SERIAL(eventName);
  SERIALIZE_ALL(eventName)
  ITEM_TYPE_INTERFACE;
};
struct Assembled {
  CreatureId SERIAL(creature);
  optional<ViewId> SERIAL(viewId);
  SERIALIZE_ALL(creature, viewId)
  ITEM_TYPE_INTERFACE;
};
using Simple = CustomItemId;
MAKE_VARIANT(ItemTypeVariant, Scroll, Potion, Mushroom, Amulet, Ring, TechBook, TrapItem,  Intrinsic, Glyph, Simple, FireScroll, Poem, EventPoem, Assembled);

}

class ItemTypeVariant : public ItemTypes::ItemTypeVariant {
  public:
  using ItemTypes::ItemTypeVariant::ItemTypeVariant;
};
