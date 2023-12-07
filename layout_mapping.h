#pragma once

#include "stdafx.h"
#include "furniture_type.h"
#include "furniture_layer.h"
#include "item_list_id.h"
#include "tile_gas_type.h"
#include "creature_id.h"
#include "pretty_archive.h"

namespace LayoutActions {
using Place = FurnitureType;
using Flag = SquareAttrib;
struct RemoveFlag {
  SquareAttrib SERIAL(flag);
  SERIALIZE_ALL(flag)
};
enum class StairDirection;
struct Stairs {
  StairDirection SERIAL(dir);
  FurnitureType SERIAL(type);
  int SERIAL(index);
  SERIALIZE_ALL(dir, type, index)
};
struct Shop {
  int SERIAL(index);
  SERIALIZE_ALL(index)
};
using Items = ItemListId;
struct LayoutAction;
using Chain = vector<LayoutAction>;
using ClearFurniture = EmptyStruct<struct ClearFurniturTag>;
using ClearLayer = FurnitureLayer;
using InsideFurniture = EmptyStruct<struct InsideFurnitureTag>;
using OutsideFurniture = EmptyStruct<struct OutsideFurnitureTag>;
using AddGas = TileGasType;
struct HostileCreature {
  CreatureId SERIAL(id);
  SERIALIZE_ALL(id)
};
struct AlliedPrisoner {
  CreatureId SERIAL(id);
  SERIALIZE_ALL(id)
};
struct Stockpile {
  int SERIAL(index);
  SERIALIZE_ALL(index)
};
struct PlaceHostile {
  FurnitureType SERIAL(type);
  SERIALIZE_ALL(type)
};
#define VARIANT_TYPES_LIST\
  X(Place, 0)\
  X(Flag, 1)\
  X(RemoveFlag, 2)\
  X(Stairs, 3)\
  X(Chain, 4)\
  X(ClearFurniture, 5)\
  X(Stockpile, 6)\
  X(ClearLayer, 7)\
  X(OutsideFurniture, 8)\
  X(InsideFurniture, 9)\
  X(Items, 10)\
  X(Shop, 11)\
  X(PlaceHostile, 12)\
  X(AddGas, 13)\
  X(HostileCreature, 14)\
  X(AlliedPrisoner, 15)\

#define VARIANT_NAME LayoutAction

#include "gen_variant.h"
#include "gen_variant_serialize.h"
#define DEFAULT_ELEM "Chain"
inline
#include "gen_variant_serialize_pretty.h"
#undef DEFAULT_ELEM
#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

using LayoutActions::LayoutAction;

struct LayoutMapping {
  map<string, LayoutAction> SERIAL(actions);
  SERIALIZE_ALL(actions)
};

RICH_ENUM(
    LayoutActions::StairDirection,
    UP, DOWN
);
