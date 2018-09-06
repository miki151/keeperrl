#include "furniture_fx.h"
#include "furniture_type.h"
#include "util.h"

using Type = FurnitureType;
using Info = FurnitureFXInfo;

// TODO: make FurnitureClass ?
static bool isWall(Type type) {
  return isOneOf(type, Type::MOUNTAIN, Type::MOUNTAIN2, Type::ADAMANTIUM_ORE, Type::IRON_ORE, Type::STONE,
                 Type::GOLD_ORE, Type::DUNGEON_WALL, Type::DUNGEON_WALL2);
}

optional<Info> destroyFXInfo(Type type) {
  if (isWall(type)) {
    // TODO: different colors for different types
    Info out{FXName::ROCK_CLOUD, Color::WHITE};
    return out;
  }

  return none;
}

optional<Info> tryDestroyFXInfo(Type type) {
  if (isWall(type)) {
    // TODO: different colors for different types
    return Info{FXName::ROCK_SPLINTERS, Color::WHITE};
  } else if (isOneOf(type, Type::CANIF_TREE, Type::DECID_TREE, Type::BUSH))
    return Info{FXName::WOOD_SPLINTERS, Color::WHITE};
  return none;
}

optional<Info> walkIntoFXInfo(Type type) {
  if (isOneOf(type, Type::WATER, Type::SHALLOW_WATER1, Type::SHALLOW_WATER2))
    return Info{FXName::WATER_SPLASH, Color::WHITE};
  return none;
}
optional<Info> walkOverFXInfo(Type type) {
  if (type == Type::SAND)
    return Info{FXName::SAND_DUST, Color::WHITE};
  return none;
}
