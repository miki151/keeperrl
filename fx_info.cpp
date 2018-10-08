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
    case Name::SPEED:
      return {FXName::SPEED};
    case Name::SLEEP:
      return {FXName::SLEEP};
    case Name::FLYING:
      return {FXName::FLYING};
    case Name::FIRE_SPHERE:
      return {FXName::FIRE_SPHERE};
    case Name::DEBUFF_RED:
      return {FXName::DEBUFF, Color::RED, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_GREEN:
      return {FXName::DEBUFF, Color::GREEN, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_BLACK:
      return {FXName::DEBUFF, Color::BLACK, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_WHITE:
      return {FXName::DEBUFF, Color::WHITE, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_GRAY:
      return {FXName::DEBUFF, Color::GRAY, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_PINK:
      return {FXName::DEBUFF, Color::PINK, 0.0f, FXStackId::debuff};
    case Name::DEBUFF_ORANGE:
      return {FXName::DEBUFF, Color::ORANGE, 0.0f, FXStackId::debuff};
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
    case Name::JEWELER:
      return {FXName::JEWELER, Color(253, 247, 140)};
  }
}

optional<FXInfo> getOverlayFXInfo(ViewId id) {
  if (isOneOf(id, ViewId::GOLD_ORE, ViewId::GOLD, ViewId::THRONE, ViewId::MINION_STATUE))
    return FXInfo{FXName::GLITTERING, Color(253, 247, 172)};
  if (id == ViewId::ADAMANTIUM_ORE)
    return FXInfo{FXName::GLITTERING, Color::LIGHT_BLUE};
  return none;
}

using FType = FurnitureType;

// TODO: make FurnitureClass ?
static bool isMountain2(FType type) {
  return isOneOf(type, FType::MOUNTAIN2, FType::IRON_ORE, FType::GOLD_ORE, FType::ADAMANTIUM_ORE, FType::STONE,
                 FType::DUNGEON_WALL2);
}

static bool isMountain(FType type) {
  return isOneOf(type, FType::MOUNTAIN, FType::DUNGEON_WALL);
}

static bool isTree(FType type) {
  return isOneOf(type, FType::CANIF_TREE, FType::DECID_TREE, FType::BUSH);
}

// TODO: EnumMap mapping item to destruction effect ?
static bool isWoodenFurniture(FType type) {
  return isOneOf(type, FType::WOOD_DOOR, FType::WOOD_WALL, FType::BOOKCASE_WOOD, FType::TRAINING_WOOD, FType::WORKSHOP,
                 FType::JEWELER, FType::BOOKCASE_GOLD, FType::BOOKCASE_IRON, FType::ARCHERY_RANGE, FType::BARRICADE,
                 FType::KEEPER_BOARD, FType::EYEBALL, FType::WHIPPING_POST, FType::GALLOWS, FType::BED1, FType::BED2,
                 FType::BED3, FType::COFFIN1, FType::BEAST_CAGE, FType::TREASURE_CHEST);
}

static bool isSmallWoodenFurniture(FType type) {
  return isOneOf(type, FType::IMPALED_HEAD);
}

static bool isGrayFurniture(FType type) {
  return isOneOf(type, FType::BOOKCASE_IRON, FType::TRAINING_IRON, FType::IRON_DOOR, FType::TRAINING_ADA,
                 FType::ADA_DOOR, FType::LABORATORY, FType::FORGE, FType::STONE_MINION_STATUE, FType::FOUNTAIN,
                 FType::PORTAL, FType::COFFIN2, FType::COFFIN3, FType::TORTURE_TABLE);
}

static bool isGoldFurniture(FType type) {
  return isOneOf(type, FType::MINION_STATUE, FType::THRONE, FType::DEMON_SHRINE);
}

optional<FXInfo> destroyFXInfo(FType type) {
  if (isMountain(type))
    return FXInfo{FXName::ROCK_CLOUD, Color(220, 210, 180)};
  else if (isMountain2(type) || type == FType::BOULDER_TRAP)
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

optional<FXInfo> tryDestroyFXInfo(FType type) {
  if (isMountain(type))
    return FXInfo{FXName::ROCK_SPLINTERS, Color(220, 210, 180)};
  else if (isMountain2(type))
    return FXInfo{FXName::ROCK_SPLINTERS, Color(200, 200, 200)};
  else if (isTree(type))
    return FXInfo{FXName::WOOD_SPLINTERS, Color::WHITE};
  return none;
}

optional<FXInfo> walkIntoFXInfo(FType type) {
  if (isOneOf(type, FType::WATER, FType::SHALLOW_WATER1, FType::SHALLOW_WATER2))
    // TODO: color should depend on depth ?
    return FXInfo{FXName::WATER_SPLASH, Color(82, 148, 225)};
  return none;
}

optional<FXInfo> walkOverFXInfo(FType type) {
  if (type == FType::SAND)
    return FXInfo{FXName::SAND_DUST, Color(255, 229, 178)};
  return none;
}

optional<FXVariantName> getFurnitureUsageFX(FurnitureType type) {
  switch (type) {
    case FurnitureType::JEWELER:
      return FXVariantName::JEWELER;
    case FurnitureType::LABORATORY:
      return FXVariantName::LABORATORY;
    case FurnitureType::WORKSHOP:
      return FXVariantName::WORKSHOP;
    case FurnitureType::FORGE:
      return FXVariantName::FORGE;
    default:
      return none;
  }
}
