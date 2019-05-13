#include "stdafx.h"
#include "fx_info.h"
#include "fx_name.h"
#include "color.h"

#include "workshop_type.h"
#include "fx_variant_name.h"
#include "furniture_type.h"
#include "view_id.h"

FXInfo getFXInfo(FXVariantName var) {
  using Name = FXVariantName;
  switch (var) {
    case Name::BLIND:
      return {FXName::BLIND};
    case Name::SLEEP:
      return {FXName::SLEEP};
    case Name::FLYING:
      return {FXName::FLYING};
    case Name::FIRE_SPHERE:
      return {FXName::FIRE_SPHERE};
    case Name::FIRE:
      return {FXName::FIRE};
    case Name::SPIRAL_BLUE:
      return {FXName::SPIRAL, Color::BLUE};
    case Name::SPIRAL_GREEN:
      return {FXName::SPIRAL, Color::f(0.7, 1.0, 0.6)};
    case Name::LABORATORY:
      return {FXName::LABORATORY, Color::GREEN};
    case Name::FORGE:
      return {FXName::FORGE, Color(252, 142, 30)};
    case Name::WORKSHOP:
      return {FXName::WORKSHOP};
    case Name::JEWELLER:
      return {FXName::JEWELLER, Color(253, 247, 140)};

    case Name::BUFF_RED:     return {FXName::BUFF,   Color(250, 40, 40), 0.0f, FXStackId::buff};
    case Name::BUFF_YELLOW:  return {FXName::BUFF,   Color(255, 255, 100), 0.0f, FXStackId::buff};
    case Name::BUFF_BLUE:    return {FXName::BUFF,   Color(70, 130, 225), 0.0f, FXStackId::buff};
    case Name::BUFF_PINK:    return {FXName::BUFF,   Color(255, 100, 255), 0.0f, FXStackId::buff};
    case Name::BUFF_ORANGE:  return {FXName::BUFF,   Color::ORANGE, 0.0f, FXStackId::buff};
    case Name::BUFF_GREEN2:  return {FXName::BUFF,   Color(80, 255, 120), 0.0f, FXStackId::buff};
    case Name::BUFF_SKY_BLUE:return {FXName::BUFF,   Color::SKY_BLUE, 0.0f, FXStackId::buff};
    case Name::BUFF_BROWN:   return {FXName::BUFF,   Color::BROWN, 0.0f, FXStackId::buff};

    case Name::DEBUFF_RED:   return {FXName::DEBUFF, Color(190, 30, 30), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_BLUE:  return {FXName::DEBUFF, Color(30, 60, 230), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_GREEN1:return {FXName::DEBUFF, Color(0, 160, 30), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_GREEN2:return {FXName::DEBUFF, Color(80, 255, 120), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_PINK:  return {FXName::DEBUFF, Color(160, 10, 180), 0.0f, FXStackId::debuff};
    case Name::DEBUFF_ORANGE:return {FXName::DEBUFF, Color::ORANGE, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_BROWN:   return {FXName::DEBUFF, Color::BROWN, 0.0f, FXStackId::debuff};
  }
}

optional<FXInfo> getOverlayFXInfo(ViewId id) {
  if (isOneOf(id, ViewId("gold_ore"), ViewId("gold"), ViewId("throne"), ViewId("minion_statue"), ViewId("demon_shrine")))
    return FXInfo{FXName::GLITTERING, Color(253, 247, 172)};
  if (id == ViewId("adamantium_ore"))
    return FXInfo{FXName::GLITTERING, Color::LIGHT_BLUE};
  if (id == ViewId("magma"))
    return FXInfo{FXName::MAGMA_FIRE};
  return none;
}

// TODO: make FurnitureClass ?
static bool isMountain2(FurnitureType type) {
  return isOneOf(type, FurnitureType("MOUNTAIN2"), FurnitureType("IRON_ORE"), FurnitureType("GOLD_ORE"), FurnitureType("ADAMANTIUM_ORE"), FurnitureType("STONE"),
                 FurnitureType("DUNGEON_WALL2"));
}

static bool isMountain(FurnitureType type) {
  return isOneOf(type, FurnitureType("MOUNTAIN"), FurnitureType("DUNGEON_WALL"));
}

static bool isTree(FurnitureType type) {
  return isOneOf(type, FurnitureType("CANIF_TREE"), FurnitureType("DECID_TREE"), FurnitureType("BUSH"));
}

// TODO: EnumMap mapping item to destruction effect ?
static bool isWoodenFurniture(FurnitureType type) {
  return isOneOf(type, FurnitureType("WOOD_DOOR"), FurnitureType("WOOD_WALL"), FurnitureType("BOOKCASE_WOOD"), FurnitureType("TRAINING_WOOD"), FurnitureType("WORKSHOP"),
                 FurnitureType("JEWELLER"), FurnitureType("ARCHERY_RANGE"), FurnitureType("BARRICADE"), FurnitureType("KEEPER_BOARD"), FurnitureType("EYEBALL"),
                 FurnitureType("WHIPPING_POST"), FurnitureType("GALLOWS"), FurnitureType("BED1"), FurnitureType("BED2"), FurnitureType("BED3"), FurnitureType("COFFIN1"),
                 FurnitureType("BEAST_CAGE"), FurnitureType("TREASURE_CHEST"));
}

static bool isSmallWoodenFurniture(FurnitureType type) {
  return isOneOf(type, FurnitureType("IMPALED_HEAD"));
}

static bool isGrayFurniture(FurnitureType type) {
  return isOneOf(type, FurnitureType("BOOKCASE_IRON"), FurnitureType("TRAINING_IRON"), FurnitureType("IRON_DOOR"), FurnitureType("TRAINING_ADA"),
                 FurnitureType("ADA_DOOR"), FurnitureType("LABORATORY"), FurnitureType("FORGE"), FurnitureType("STONE_MINION_STATUE"), FurnitureType("FOUNTAIN"),
                 FurnitureType("PORTAL"), FurnitureType("COFFIN2"), FurnitureType("TORTURE_TABLE"), FurnitureType("BOOKCASE_IRON"), FurnitureType("GRAVE"));
}

static bool isGoldFurniture(FurnitureType type) {
  return isOneOf(type, FurnitureType("MINION_STATUE"), FurnitureType("THRONE"), FurnitureType("DEMON_SHRINE"), FurnitureType("COFFIN3"), FurnitureType("BOOKCASE_GOLD"));
}

optional<FXInfo> destroyFXInfo(FurnitureType type) {
  if (isMountain(type))
    return FXInfo{FXName::ROCK_CLOUD, Color(220, 210, 180)};
  else if (isMountain2(type) || type == FurnitureType("BOULDER_TRAP"))
    return FXInfo{FXName::ROCK_CLOUD, Color(200, 200, 200)};
  else if (isWoodenFurniture(type))
    return FXInfo{FXName::DESTROY_FURNITURE, Color(120, 87, 46)};
  else if (isGrayFurniture(type))
    return FXInfo{FXName::DESTROY_FURNITURE, Color(120, 120, 120)};
  else if (isGoldFurniture(type))
    return FXInfo{FXName::DESTROY_FURNITURE, Color(190, 190, 40)};
  else if (isSmallWoodenFurniture(type))
    return FXInfo{FXName::WOOD_SPLINTERS, Color::WHITE};
  return none;
}

optional<FXInfo> tryDestroyFXInfo(FurnitureType type) {
  if (isMountain(type))
    return FXInfo{FXName::ROCK_SPLINTERS, Color(220, 210, 180)};
  else if (isMountain2(type))
    return FXInfo{FXName::ROCK_SPLINTERS, Color(200, 200, 200)};
  else if (isTree(type))
    return FXInfo{FXName::WOOD_SPLINTERS, Color::WHITE};
  return none;
}

optional<FXInfo> walkIntoFXInfo(FurnitureType type) {
  if (isOneOf(type, FurnitureType("WATER"), FurnitureType("SHALLOW_WATER1"), FurnitureType("SHALLOW_WATER2")))
    // TODO: color should depend on depth ?
    return FXInfo{FXName::WATER_SPLASH, Color(82, 148, 225)};
  return none;
}

optional<FXInfo> walkOverFXInfo(FurnitureType type) {
  if (type == FurnitureType("SAND"))
    return FXInfo{FXName::SAND_DUST, Color(255, 229, 178)};
  return none;
}

optional<FXVariantName> getFurnitureUsageFX(FurnitureType type) {
  if (type == FurnitureType("JEWELLER"))
    return FXVariantName::JEWELLER;
  if (type == FurnitureType("LABORATORY"))
    return FXVariantName::LABORATORY;
  if (type == FurnitureType("WORKSHOP"))
    return FXVariantName::WORKSHOP;
  if (type == FurnitureType("FORGE"))
    return FXVariantName::FORGE;
  return none;
}
