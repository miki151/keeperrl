#pragma once

#include "enum_variant.h"
#include "effect.h"
#include "util.h"
#include "weapon_info.h"
#include "item_prefix.h"
#include "item_upgrade_info.h"
#include "view_id.h"
#include "tech_id.h"
#include "custom_item_id.h"

class ItemAttributes;
class ContentFactory;


#define ITEM_TYPE_INTERFACE\
  ItemAttributes getAttributes(const ContentFactory*) const

#define SIMPLE_ITEM(Name) \
  struct Name { \
    ITEM_TYPE_INTERFACE;\
    COMPARE_ALL()\
  }

class ItemType {
  public:
  struct Intrinsic {
    ViewId viewId;
    string name;
    int damage;
    WeaponInfo weaponInfo;
    COMPARE_ALL(viewId, name, damage, weaponInfo)
    ITEM_TYPE_INTERFACE;
  };
  static ItemType touch(Effect victimEffect, vector<Effect> attackerEffect = {});
  static ItemType legs(int damage);
  static ItemType claws(int damage);
  static ItemType beak(int damage);
  static ItemType fists(int damage);
  static ItemType fists(int damage, Effect);
  static ItemType fangs(int damage);
  static ItemType fangs(int damage, Effect);
  static ItemType spellHit(int damage);
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
  using Simple = CustomItemId;
  MAKE_VARIANT(Type, Scroll, Potion, Mushroom, Amulet, Ring, TechBook, TrapItem,  Intrinsic, Glyph, Simple, FireScroll);

  template <typename T>
  ItemType(T&& t) : type(std::forward<T>(t)) {}
  ItemType(const ItemType&);
  ItemType(ItemType&);
  ItemType(ItemType&&);
  ItemType();
  ItemType& operator = (const ItemType&);
  ItemType& operator = (ItemType&&);
  bool operator == (const ItemType&) const;
  bool operator != (const ItemType&) const;

  ItemType& setPrefixChance(double chance);

  template <typename T>
  optional<T> get() const {
    return type.getValueMaybe<T>();
  }

  template <class Archive>
  void serialize(Archive&, const unsigned int);

  PItem get(const ContentFactory*) const;
  vector<PItem> get(int, const ContentFactory*) const;
  ~ItemType(){}

  private:
  Type SERIAL(type);
  ItemAttributes getAttributes(const ContentFactory*) const;
  double SERIAL(prefixChance) = 0.0;
};
