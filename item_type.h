#pragma once

#include "enum_variant.h"
#include "effect.h"
#include "util.h"

RICH_ENUM(
    ItemId,
    SPECIAL_KNIFE,
    KNIFE,
    SPEAR,
    SWORD,
    STEEL_SWORD,
    SPECIAL_SWORD,
    ELVEN_SWORD,
    SPECIAL_ELVEN_SWORD,
    BATTLE_AXE,
    STEEL_BATTLE_AXE,
    SPECIAL_BATTLE_AXE,
    WAR_HAMMER,
    SPECIAL_WAR_HAMMER,
    CLUB,
    HEAVY_CLUB,
    WOODEN_STAFF,
    IRON_STAFF,
    SCYTHE,
    BOW,
    ELVEN_BOW,

    LEATHER_ARMOR,
    LEATHER_HELM,
    TELEPATHY_HELM,
    CHAIN_ARMOR,
    STEEL_ARMOR,
    IRON_HELM,
    LEATHER_BOOTS,
    IRON_BOOTS,
    SPEED_BOOTS,
    LEVITATION_BOOTS,
    LEATHER_GLOVES,
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
    STEEL_INGOT,
    GOLD_PIECE,
    WOOD_PLANK,
    BONE,

    RANDOM_TECH_BOOK,
    TECH_BOOK,

    TRAP_ITEM,
    AUTOMATON_ITEM
);

class ItemType : public EnumVariant<ItemId, TYPES(Effect, TrapType, LastingEffect, TechId),
        ASSIGN(Effect, ItemId::SCROLL, ItemId::POTION, ItemId::MUSHROOM),
        ASSIGN(TrapType, ItemId::TRAP_ITEM),
        ASSIGN(LastingEffect, ItemId::RING),
        ASSIGN(TechId, ItemId::TECH_BOOK)> {
  using EnumVariant::EnumVariant;
};

