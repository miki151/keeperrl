#ifndef _ITEM_TYPE_H
#define _ITEM_TYPE_H

#include "enum_variant.h"
#include "effect_type.h"

enum class ItemId {
  SPECIAL_KNIFE,
  KNIFE,
  SPEAR,
  SWORD,
  SPECIAL_SWORD,
  ELVEN_SWORD,
  SPECIAL_ELVEN_SWORD,
  BATTLE_AXE,
  SPECIAL_BATTLE_AXE,
  WAR_HAMMER,
  SPECIAL_WAR_HAMMER,
  CLUB,
  HEAVY_CLUB,
  SCYTHE,
  BOW,
  ARROW,

  LEATHER_ARMOR,
  LEATHER_HELM,
  TELEPATHY_HELM,
  CHAIN_ARMOR,
  IRON_HELM,
  LEATHER_BOOTS,
  IRON_BOOTS,
  SPEED_BOOTS,
  LEVITATION_BOOTS,
  LEATHER_GLOVES,
  DEXTERITY_GLOVES,
  STRENGTH_GLOVES,
  ROBE,

  SCROLL,
  FIRE_SCROLL,
  POTION,
  MUSHROOM,

  WARNING_AMULET,
  HEALING_AMULET,
  DEFENSE_AMULET,

  RING,

  FIRST_AID_KIT,
  ROCK,
  IRON_ORE,
  GOLD_PIECE,
  WOOD_PLANK,
  BONE,

  RANDOM_TECH_BOOK,
  TECH_BOOK,

  TRAP_ITEM,
  BOULDER_TRAP_ITEM,
  AUTOMATON_ITEM,
};

struct TrapInfo {
  TrapType SERIAL(trapType);
  EffectType SERIAL(effectType);
  bool SERIAL(alwaysVisible);
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(trapType) & SVAR(effectType) & SVAR(alwaysVisible);
  }
};

class ItemType : public EnumVariant<ItemId, TYPES(EffectType, TrapInfo, LastingEffect, TechId),
        ASSIGN(EffectType, ItemId::SCROLL, ItemId::POTION, ItemId::MUSHROOM),
        ASSIGN(TrapInfo, ItemId::TRAP_ITEM),
        ASSIGN(LastingEffect, ItemId::RING),
        ASSIGN(TechId, ItemId::TECH_BOOK)> {
  using EnumVariant::EnumVariant;
};

#endif
