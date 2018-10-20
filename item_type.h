#pragma once

#include "enum_variant.h"
#include "effect.h"
#include "util.h"
#include "weapon_info.h"
#include "item_prefix.h"

class ItemAttributes;


#define ITEM_TYPE_INTERFACE\
  ItemAttributes getAttributes() const

#define SIMPLE_ITEM(Name) \
  struct Name { \
    SERIALIZE_EMPTY()\
    ITEM_TYPE_INTERFACE;\
  }

class ItemType {
  public:
  SIMPLE_ITEM(Knife);
  SIMPLE_ITEM(UnicornHorn);
  SIMPLE_ITEM(Spear);
  SIMPLE_ITEM(Sword);
  SIMPLE_ITEM(AdaSword);
  SIMPLE_ITEM(ElvenSword);
  SIMPLE_ITEM(BattleAxe);
  SIMPLE_ITEM(AdaBattleAxe);
  SIMPLE_ITEM(WarHammer);
  SIMPLE_ITEM(AdaWarHammer);
  SIMPLE_ITEM(Club);
  SIMPLE_ITEM(HeavyClub);
  SIMPLE_ITEM(WoodenStaff);
  SIMPLE_ITEM(IronStaff);
  SIMPLE_ITEM(GoldenStaff);
  SIMPLE_ITEM(Scythe);
  SIMPLE_ITEM(Bow);
  SIMPLE_ITEM(ElvenBow);
  SIMPLE_ITEM(Torch);

  SIMPLE_ITEM(LeatherArmor);
  SIMPLE_ITEM(ChainArmor);
  SIMPLE_ITEM(AdaArmor);
  SIMPLE_ITEM(LeatherHelm);
  SIMPLE_ITEM(IronHelm);
  SIMPLE_ITEM(AdaHelm);
  SIMPLE_ITEM(LeatherBoots);
  SIMPLE_ITEM(IronBoots);
  SIMPLE_ITEM(AdaBoots);
  SIMPLE_ITEM(LeatherGloves);
  SIMPLE_ITEM(IronGloves);
  SIMPLE_ITEM(AdaGloves);
  SIMPLE_ITEM(Robe);
  SIMPLE_ITEM(HalloweenCostume);
  SIMPLE_ITEM(BagOfCandies);

  struct Intrinsic {
    ViewId SERIAL(viewId);
    string SERIAL(name);
    int SERIAL(damage);
    WeaponInfo SERIAL(weaponInfo);
    SERIALIZE_ALL(viewId, name, damage, weaponInfo)
    ITEM_TYPE_INTERFACE;
  };
  static ItemType touch(Effect victimEffect, optional<Effect> attackerEffect = none);
  static ItemType legs(int damage);
  static ItemType claws(int damage);
  static ItemType beak(int damage);
  static ItemType fists(int damage);
  static ItemType fists(int damage, Effect);
  static ItemType fangs(int damage);
  static ItemType fangs(int damage, Effect);
  static ItemType spellHit(int damage);
  struct Scroll {
    Effect SERIAL(effect);
    SERIALIZE_ALL(effect)
    ITEM_TYPE_INTERFACE;
  };
  SIMPLE_ITEM(FireScroll);
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
  SIMPLE_ITEM(DefenseAmulet);
  struct Ring {
    LastingEffect SERIAL(lastingEffect);
    SERIALIZE_ALL(lastingEffect)
    ITEM_TYPE_INTERFACE;
  };

  SIMPLE_ITEM(FirstAidKit);
  SIMPLE_ITEM(Rock);
  SIMPLE_ITEM(IronOre);
  SIMPLE_ITEM(AdaOre);
  SIMPLE_ITEM(GoldPiece);
  SIMPLE_ITEM(WoodPlank);
  SIMPLE_ITEM(Bone);
  SIMPLE_ITEM(RandomTechBook);
  struct TechBook {
    TechId techId;
    COMPARE_ALL(techId)
    ITEM_TYPE_INTERFACE;
  };
  struct TrapItem {
    TrapType SERIAL(trapType);
    SERIALIZE_ALL(trapType)
    ITEM_TYPE_INTERFACE;
  };
  SIMPLE_ITEM(AutomatonItem);

  MAKE_VARIANT(Type, Knife, Spear, Sword, AdaSword, ElvenSword,
      BattleAxe, AdaBattleAxe, WarHammer, AdaWarHammer, Club, HeavyClub, WoodenStaff, IronStaff, GoldenStaff,
      Scythe, Bow, ElvenBow, LeatherArmor, LeatherHelm, IronHelm, AdaHe`lm, ChainArmor, AdaArmor, LeatherBoots,
      IronBoots, AdaBoots, LeatherGloves, IronGloves, AdaGloves, Robe, Scroll, FireScroll, Potion,
      Mushroom, Amulet, DefenseAmulet, Ring, FirstAidKit, Rock, IronOre, AdaOre, GoldPiece,
      WoodPlank, Bone, RandomTechBook, TechBook, TrapItem, AutomatonItem, BagOfCandies, HalloweenCostume,
      UnicornHorn, Intrinsic, Torch);

  template <typename T>
  ItemType(T&& t) : type(std::forward<T>(t)) {}
  ItemType(const ItemType&);
  ItemType(ItemType&);
  ItemType(ItemType&&);
  ItemType();
  ItemType& operator = (const ItemType&);
  ItemType& operator = (ItemType&&);

  ItemType& setPrefixChance(double chance);

  template <typename T>
  bool isType() const {
    return type.contains<T>();
  }

  template <class Archive>
  void serialize(Archive&, const unsigned int);

  PItem get() const;
  vector<PItem> get(int) const;
  ~ItemType(){}

  private:
  Type SERIAL(type);
  ItemAttributes getAttributes() const;
  double SERIAL(prefixChance) = 0.0;
};
