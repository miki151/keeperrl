#pragma once

#include "enum_variant.h"
#include "effect.h"
#include "util.h"

class ItemAttributes;


#define ITEM_TYPE_INTERFACE\
  ItemAttributes getAttributes() const

#define SIMPLE_ITEM(Name) \
  struct Name { \
    COMPARE_ALL()\
    ITEM_TYPE_INTERFACE;\
  }

class ItemType {
  public:
  SIMPLE_ITEM(SpecialKnife);
  SIMPLE_ITEM(Knife);
  SIMPLE_ITEM(Spear);
  SIMPLE_ITEM(Sword);
  SIMPLE_ITEM(SteelSword);
  SIMPLE_ITEM(SpecialSword);
  SIMPLE_ITEM(ElvenSword);
  SIMPLE_ITEM(SpecialElvenSword);
  SIMPLE_ITEM(BattleAxe);
  SIMPLE_ITEM(SteelBattleAxe);
  SIMPLE_ITEM(SpecialBattleAxe);
  SIMPLE_ITEM(WarHammer);
  SIMPLE_ITEM(SpecialWarHammer);
  SIMPLE_ITEM(Club);
  SIMPLE_ITEM(HeavyClub);
  SIMPLE_ITEM(WoodenStaff);
  SIMPLE_ITEM(IronStaff);
  SIMPLE_ITEM(Scythe);
  SIMPLE_ITEM(Bow);
  SIMPLE_ITEM(ElvenBow);

  SIMPLE_ITEM(LeatherArmor);
  SIMPLE_ITEM(LeatherHelm);
  SIMPLE_ITEM(TelepathyHelm);
  SIMPLE_ITEM(ChainArmor);
  SIMPLE_ITEM(SteelArmor);
  SIMPLE_ITEM(IronHelm);
  SIMPLE_ITEM(LeatherBoots);
  SIMPLE_ITEM(IronBoots);
  SIMPLE_ITEM(SpeedBoots);
  SIMPLE_ITEM(LevitationBoots);
  SIMPLE_ITEM(LeatherGloves);
  SIMPLE_ITEM(StrengthGloves);
  SIMPLE_ITEM(Robe);

  struct Scroll {
    Effect effect;
    COMPARE_ALL(effect)
    ITEM_TYPE_INTERFACE;
  };
  SIMPLE_ITEM(FireScroll);
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
  SIMPLE_ITEM(WarningAmulet);
  SIMPLE_ITEM(HealingAmulet);
  SIMPLE_ITEM(DefenseAmulet);
  struct Ring {
    LastingEffect lastingEffect;
    COMPARE_ALL(lastingEffect)
    ITEM_TYPE_INTERFACE;
  };

  SIMPLE_ITEM(FirstAidKit);
  SIMPLE_ITEM(Rock);
  SIMPLE_ITEM(IronOre);
  SIMPLE_ITEM(SteelIngot);
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
    TrapType trapType;
    COMPARE_ALL(trapType)
    ITEM_TYPE_INTERFACE;
  };
  SIMPLE_ITEM(AutomatonItem);

  MAKE_VARIANT(Type, SpecialKnife, Knife, Spear, Sword, SteelSword, SpecialSword, ElvenSword, SpecialElvenSword,
      BattleAxe, SteelBattleAxe, SpecialBattleAxe, WarHammer, SpecialWarHammer, Club, HeavyClub, WoodenStaff, IronStaff,
      Scythe, Bow, ElvenBow, LeatherArmor, LeatherHelm, TelepathyHelm, ChainArmor, SteelArmor, IronHelm, LeatherBoots,
      IronBoots, SpeedBoots, LevitationBoots, LeatherGloves, StrengthGloves, Robe, Scroll, FireScroll, Potion,
      Mushroom, WarningAmulet, HealingAmulet, DefenseAmulet, Ring, FirstAidKit, Rock, IronOre, SteelIngot, GoldPiece,
      WoodPlank, Bone, RandomTechBook, TechBook, TrapItem, AutomatonItem);

  template <typename T>
  ItemType(T&& t) : type(std::forward<T>(t)) {}
  ItemType(const ItemType&) = default;
  ItemType(ItemType&) = default;
  ItemType(ItemType&&) = default;
  ItemType() {}
  ItemType& operator = (const ItemType&) = default;
  ItemType& operator = (ItemType&&) = default;

  template <typename T>
  bool isType() const {
    return type.contains<T>();
  }

  COMPARE_ALL(type)

  PItem get() const;
  vector<PItem> get(int) const;

  private:
  Type type;
  ItemAttributes getAttributes() const;
};
