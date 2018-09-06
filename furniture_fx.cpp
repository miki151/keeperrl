#include "furniture_fx.h"
#include "furniture_type.h"
#include "util.h"

using Type = FurnitureType;
using Info = FurnitureFXInfo;

// TODO: make FurnitureClass ?
static bool isMountain2(Type type) {
  return isOneOf(type, Type::MOUNTAIN2, Type::IRON_ORE, Type::GOLD_ORE, Type::ADAMANTIUM_ORE, Type::STONE,
                 Type::DUNGEON_WALL2);
}

static bool isMountain(Type type) {
  return isOneOf(type, Type::MOUNTAIN, Type::DUNGEON_WALL);
}

optional<Info> destroyFXInfo(Type type) {
  if (isMountain(type))
    return Info{FXName::ROCK_CLOUD, Color(220, 210, 180)};
  else if (isMountain2(type))
    return Info{FXName::ROCK_CLOUD, Color(200, 200, 200)};
  return none;
}

optional<Info> tryDestroyFXInfo(Type type) {
  if (isMountain(type))
    return Info{FXName::ROCK_SPLINTERS, Color(220, 210, 180)};
  else if (isMountain2(type))
    return Info{FXName::ROCK_SPLINTERS, Color(200, 200, 200)};
  else if (isOneOf(type, Type::CANIF_TREE, Type::DECID_TREE, Type::BUSH))
    return Info{FXName::WOOD_SPLINTERS, Color::WHITE};
  return none;
}

optional<Info> walkIntoFXInfo(Type type) {
  if (isOneOf(type, Type::WATER, Type::SHALLOW_WATER1, Type::SHALLOW_WATER2))
    // TODO: color should depend on depth ?
    return Info{FXName::WATER_SPLASH, Color(102, 178, 255)};
  return none;
}

optional<Info> walkOverFXInfo(Type type) {
  if (type == Type::SAND)
    return Info{FXName::SAND_DUST, Color(255, 229, 178)};
  return none;
}
